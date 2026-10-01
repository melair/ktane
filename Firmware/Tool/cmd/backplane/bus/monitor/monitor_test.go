package monitor

import (
	"strings"
	"testing"
	"time"
)

func TestDurationFlagDefaultsToIndefiniteAndParsesDuration(t *testing.T) {
	cmd := NewCommand()
	duration := cmd.Flag("duration")
	if duration == nil {
		t.Fatal("monitor command is missing duration flag")
	}
	if duration.DefValue != "0s" {
		t.Fatalf("duration default %q, want %q", duration.DefValue, "0s")
	}

	if err := cmd.ParseFlags([]string{"--duration", "1m30s"}); err != nil {
		t.Fatal(err)
	}
	parsed, err := cmd.Flags().GetDuration("duration")
	if err != nil {
		t.Fatal(err)
	}
	if parsed != 90*time.Second {
		t.Fatalf("parsed duration %s, want %s", parsed, 90*time.Second)
	}
}

func TestNegativeDurationRejectedBeforeOpeningPort(t *testing.T) {
	cmd := NewCommand()
	cmd.SetArgs([]string{"--port", "unused", "--duration", "-1s"})

	err := cmd.Execute()
	if err == nil || !strings.Contains(err.Error(), "--duration must not be negative") {
		t.Fatalf("got error %v, want negative duration error", err)
	}
}

func TestCOBSDecoderHandlesFragmentedAndConsecutiveFrames(t *testing.T) {
	decoder := cobsDecoder{frame: make([]byte, 0, maxEncodedSize)}
	first := encodeCOBSForTest([]byte{0x01, 0x00, 0x02})
	second := encodeCOBSForTest([]byte{0x03, 0x04})

	if packets := decoder.service([]byte{0x99, 0x00}); len(packets) != 0 {
		t.Fatalf("got %d packets before a complete frame, want 0", len(packets))
	}
	if packets := decoder.service(first[:2]); len(packets) != 0 {
		t.Fatalf("got %d packets from a partial frame, want 0", len(packets))
	}

	packets := decoder.service(append(first[2:], second...))
	if len(packets) != 2 {
		t.Fatalf("got %d packets, want 2", len(packets))
	}
	if got := string(packets[0]); got != string([]byte{0x01, 0x00, 0x02}) {
		t.Fatalf("decoded first packet %v", packets[0])
	}
	if got := string(packets[1]); got != string([]byte{0x03, 0x04}) {
		t.Fatalf("decoded second packet %v", packets[1])
	}
}

func TestCOBSDecoderDropsMalformedAndRecoversFromOversizedFrame(t *testing.T) {
	decoder := cobsDecoder{frame: make([]byte, 0, maxEncodedSize)}

	malformed := []byte{0x00, 0x04, 0x01, 0x00}
	if packets := decoder.service(malformed); len(packets) != 0 {
		t.Fatalf("got %d packets from malformed frame, want 0", len(packets))
	}

	oversized := append([]byte{0x00}, make([]byte, maxEncodedSize+1)...)
	decoder.service(oversized)
	decoder.service([]byte{0x00})
	packets := decoder.service(encodeCOBSForTest([]byte{0x01, 0x02, 0x03}))
	if len(packets) != 1 {
		t.Fatalf("got %d packets after oversized frame, want 1", len(packets))
	}
}

func TestFormatPacketTypes(t *testing.T) {
	tests := []struct {
		name string
		raw  []byte
		want []string
	}{
		{
			name: "inquiry",
			raw:  []byte{0x02, 0x00, 0x00},
			want: []string{"type=inquiry", "address=0x02", "eor=false"},
		},
		{
			name: "front status",
			raw:  []byte{0x03, 0x01, 0x01, 0x0e, 0x19, 0x34, 0x12, 0x78, 0x56},
			want: []string{
				"type=status", "address=0x03", "eor=true", "port=front",
				"power_state=tripped(0x03)", "module_detected=true",
				"current_limit_deciamps=25", "current_milliamps=4660",
				"peak_current_milliamps=22136",
			},
		},
		{
			name: "rear set enabled",
			raw:  []byte{0x04, 0x02, 0x00, 0x03},
			want: []string{"type=set_enabled", "port=rear", "enabled=true"},
		},
		{
			name: "rear set current limit",
			raw:  []byte{0x05, 0x03, 0x00, 0x01, 0x2a},
			want: []string{"type=set_current_limit", "port=rear", "current_limit_deciamps=42"},
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			packet, err := parsePacket(tt.raw)
			if err != nil {
				t.Fatal(err)
			}
			line := formatPacket(packet)
			for _, want := range tt.want {
				if !strings.Contains(line, want) {
					t.Fatalf("formatted line %q does not contain %q", line, want)
				}
			}
		})
	}
}

func TestPowerStateNames(t *testing.T) {
	tests := []struct {
		state uint8
		want  string
	}{
		{state: 0x00, want: "idle(0x00)"},
		{state: 0x01, want: "disabled(0x01)"},
		{state: 0x02, want: "active(0x02)"},
		{state: 0x03, want: "tripped(0x03)"},
	}

	for _, tt := range tests {
		if got := powerStateName(tt.state); got != tt.want {
			t.Errorf("powerStateName(0x%02x) = %q, want %q", tt.state, got, tt.want)
		}
	}
}

func TestUnknownPacketIncludesPayload(t *testing.T) {
	packet, err := parsePacket([]byte{0x01, 0xa5, 0x00, 0xde, 0xad})
	if err != nil {
		t.Fatal(err)
	}

	line := formatPacket(packet)
	for _, want := range []string{"type=unknown_0xa5", "payload=dead"} {
		if !strings.Contains(line, want) {
			t.Fatalf("formatted line %q does not contain %q", line, want)
		}
	}
}

func TestParsePacketRejectsShortPackets(t *testing.T) {
	tests := []struct {
		name string
		raw  []byte
		want string
	}{
		{name: "header", raw: []byte{0x01, 0x02}, want: "short header"},
		{name: "status", raw: []byte{0x01, 0x01, 0x00, 0x00, 0x19, 0x34, 0x12}, want: "got 7, want at least 9"},
		{name: "set enabled", raw: []byte{0x01, 0x02, 0x00}, want: "short set_enabled packet"},
		{name: "set current limit", raw: []byte{0x01, 0x03, 0x00, 0x01}, want: "short set_current_limit packet"},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			_, err := parsePacket(tt.raw)
			if err == nil || !strings.Contains(err.Error(), tt.want) {
				t.Fatalf("got error %v, want error containing %q", err, tt.want)
			}
		})
	}
}

func TestKnownPacketAcceptsTrailingBytes(t *testing.T) {
	packet, err := parsePacket([]byte{0x01, 0x00, 0x00, 0xaa})
	if err != nil {
		t.Fatal(err)
	}
	if got := len(packet.payload); got != 1 {
		t.Fatalf("payload length %d, want 1", got)
	}
}

func encodeCOBSForTest(data []byte) []byte {
	frame := make([]byte, 0, len(data)+2)
	codeIndex := 0
	frame = append(frame, 0)
	code := byte(1)

	for _, value := range data {
		if value == 0 {
			frame[codeIndex] = code
			codeIndex = len(frame)
			frame = append(frame, 0)
			code = 1
			continue
		}

		frame = append(frame, value)
		code++
		if code == 0xff {
			frame[codeIndex] = code
			codeIndex = len(frame)
			frame = append(frame, 0)
			code = 1
		}
	}

	frame[codeIndex] = code
	return append(frame, 0)
}
