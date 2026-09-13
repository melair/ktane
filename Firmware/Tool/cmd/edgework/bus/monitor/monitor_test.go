package monitor

import (
	"bytes"
	"context"
	"strings"
	"testing"
)

func TestCOBSDecoder(t *testing.T) {
	decoder := cobsDecoder{frame: make([]byte, 0, maxEncodedSize)}

	packets := decoder.service([]byte{0x99, 0x00})
	if len(packets) != 0 {
		t.Fatalf("got %d packets before sync, want 0", len(packets))
	}

	packets = decoder.service(encodeCOBSForTest([]byte{0x01, 0x00, 0x02}))
	if len(packets) != 1 {
		t.Fatalf("got %d packets, want 1", len(packets))
	}
	if string(packets[0]) != string([]byte{0x01, 0x00, 0x02}) {
		t.Fatalf("decoded %v, want [1 0 2]", packets[0])
	}
}

func TestEncodeCOBS(t *testing.T) {
	frame, err := encodeCOBS([]byte{0x01, 0x00, 0x02})
	if err != nil {
		t.Fatal(err)
	}

	decoder := cobsDecoder{synced: true, frame: make([]byte, 0, maxEncodedSize)}
	packets := decoder.service(frame)
	if len(packets) != 1 {
		t.Fatalf("got %d packets, want 1", len(packets))
	}
	if string(packets[0]) != string([]byte{0x01, 0x00, 0x02}) {
		t.Fatalf("decoded %v, want [1 0 2]", packets[0])
	}
}

func TestFormatStatusPacket(t *testing.T) {
	packet, err := parsePacket([]byte{
		0x03, 0x01, 0x01,
		0x00,
		'A', 'B', '1', '2', '3', '4',
		0x00, 0x00, 0x01,
		0x01,
	})
	if err != nil {
		t.Fatal(err)
	}

	line := formatPacket(packet)
	for _, want := range []string{
		"type=status",
		"address=0x03",
		"eor=true",
		"mode=serial(0x00)",
		"state=startup(0x01)",
		`serial="AB1234"`,
		"identify=true",
	} {
		if !strings.Contains(line, want) {
			t.Fatalf("formatted line %q does not contain %q", line, want)
		}
	}
	for _, unwanted := range []string{
		"indicator=",
		"batteries=",
		"ports=",
		"2fa=",
	} {
		if strings.Contains(line, unwanted) {
			t.Fatalf("formatted line %q contains inactive mode field %q", line, unwanted)
		}
	}
}

func TestParseHexBytes(t *testing.T) {
	got, err := parseHexBytes("01:02 03-04_05")
	if err != nil {
		t.Fatal(err)
	}
	if string(got) != string([]byte{1, 2, 3, 4, 5}) {
		t.Fatalf("got %v, want [1 2 3 4 5]", got)
	}
}

func TestPromptDisplayPacketBuildsStateFromModePrompts(t *testing.T) {
	input := make(chan string, 16)
	lines := make(chan monitorLine)
	readErrors := make(chan error)
	var out bytes.Buffer

	for _, value := range []string{
		"display",
		"0x03",
		"",
		"serial",
		"ABC123",
		"y",
	} {
		input <- value
	}

	packet, ok, err := promptPacket(context.Background(), input, lines, readErrors, &out, nil)
	if err != nil {
		t.Fatal(err)
	}
	if !ok {
		t.Fatal("prompt cancelled")
	}

	want := []byte{
		0x03, 0x02, 0x00,
		'A', 'B', 'C', '1', '2', '3',
		0x00, 0x00, 0x01,
	}
	if string(packet) != string(want) {
		t.Fatalf("packet %v, want %v", packet, want)
	}
}

func TestPromptDisplayPacketAcceptsBlankSerialCode(t *testing.T) {
	input := make(chan string, 16)
	lines := make(chan monitorLine)
	readErrors := make(chan error)
	var out bytes.Buffer

	for _, value := range []string{
		"display",
		"0x03",
		"",
		"serial",
		"      ",
		"",
	} {
		input <- value
	}

	packet, ok, err := promptPacket(context.Background(), input, lines, readErrors, &out, nil)
	if err != nil {
		t.Fatal(err)
	}
	if !ok {
		t.Fatal("prompt cancelled")
	}

	want := []byte{
		0x03, 0x02, 0x00,
		' ', ' ', ' ', ' ', ' ', ' ',
		0x00, 0x00, 0x00,
	}
	if string(packet) != string(want) {
		t.Fatalf("packet %v, want %v", packet, want)
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
