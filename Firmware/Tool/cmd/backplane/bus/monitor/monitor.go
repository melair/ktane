package monitor

import (
	"context"
	"encoding/binary"
	"encoding/hex"
	"errors"
	"fmt"
	"io"
	"os"
	"os/signal"
	"strings"
	"syscall"
	"time"

	"github.com/spf13/cobra"
	"go.bug.st/serial"
)

const (
	defaultBaudRate = 115200

	maxPacketSize  = 64
	maxEncodedSize = maxPacketSize + (maxPacketSize / 254) + 1
)

type options struct {
	port     string
	baudRate int
	duration time.Duration
}

type cobsDecoder struct {
	synced bool
	frame  []byte
}

type backplanePacket struct {
	address uint8
	opcode  uint8
	flags   uint8
	payload []byte
	raw     []byte
}

func NewCommand() *cobra.Command {
	opts := options{baudRate: defaultBaudRate}

	cmd := &cobra.Command{
		Use:   "monitor",
		Short: "Monitor COBS-encoded backplane bus traffic",
		Args:  cobra.NoArgs,
		RunE: func(cmd *cobra.Command, args []string) error {
			return run(cmd.Context(), opts, cmd.OutOrStdout())
		},
	}

	cmd.Flags().StringVar(&opts.port, "port", "", "Serial port path")
	cmd.Flags().IntVar(&opts.baudRate, "baud", opts.baudRate, "Serial baud rate")
	cmd.Flags().DurationVar(&opts.duration, "duration", 0, "Stop capture after this duration (0 runs indefinitely)")
	_ = cmd.MarkFlagRequired("port")

	return cmd
}

func run(ctx context.Context, opts options, out io.Writer) error {
	if opts.port == "" {
		return errors.New("--port is required")
	}
	if opts.baudRate <= 0 {
		return errors.New("--baud must be greater than zero")
	}
	if opts.duration < 0 {
		return errors.New("--duration must not be negative")
	}

	port, err := serial.Open(opts.port, &serial.Mode{
		BaudRate: opts.baudRate,
		DataBits: 8,
		Parity:   serial.NoParity,
		StopBits: serial.OneStopBit,
	})
	if err != nil {
		return fmt.Errorf("open serial port: %w", err)
	}
	defer port.Close()

	if err := port.SetReadTimeout(250 * time.Millisecond); err != nil {
		return fmt.Errorf("set read timeout: %w", err)
	}

	ctx, stop := signal.NotifyContext(ctx, os.Interrupt, syscall.SIGTERM)
	defer stop()
	if opts.duration > 0 {
		var cancel context.CancelFunc
		ctx, cancel = context.WithTimeout(ctx, opts.duration)
		defer cancel()
	}

	fmt.Fprintf(out, "monitoring %s at %d baud\n", opts.port, opts.baudRate)
	return readSerial(ctx, port, out)
}

func readSerial(ctx context.Context, port serial.Port, out io.Writer) error {
	decoder := cobsDecoder{frame: make([]byte, 0, maxEncodedSize)}
	buffer := make([]byte, 256)

	for {
		select {
		case <-ctx.Done():
			return nil
		default:
		}

		n, err := port.Read(buffer)
		if err != nil {
			return fmt.Errorf("read serial port: %w", err)
		}
		if n == 0 {
			continue
		}

		for _, data := range decoder.service(buffer[:n]) {
			packet, err := parsePacket(data)
			if err != nil {
				printMonitorLine(out, fmt.Sprintf("invalid len=%d raw=%s error=%v", len(data), hex.EncodeToString(data), err))
				continue
			}

			printMonitorLine(out, fmt.Sprintf("%s raw=%s", formatPacket(packet), hex.EncodeToString(packet.raw)))
		}
	}
}

func printMonitorLine(out io.Writer, text string) {
	fmt.Fprintf(out, "%s %s\n", time.Now().Format("2006-01-02 15:04:05.000"), text)
}

func (d *cobsDecoder) service(data []byte) [][]byte {
	var packets [][]byte

	for _, value := range data {
		if !d.synced {
			d.synced = value == 0
			continue
		}

		if value == 0 {
			if len(d.frame) > 0 {
				decoded, ok := decodeCOBS(d.frame)
				if ok {
					packets = append(packets, decoded)
				}
			}
			d.frame = d.frame[:0]
			continue
		}

		if len(d.frame) >= maxEncodedSize {
			d.frame = d.frame[:0]
			d.synced = false
			continue
		}

		d.frame = append(d.frame, value)
	}

	return packets
}

func decodeCOBS(frame []byte) ([]byte, bool) {
	decoded := make([]byte, 0, maxPacketSize)
	readIndex := 0

	for readIndex < len(frame) {
		code := int(frame[readIndex])
		readIndex++
		if code == 0 {
			return nil, false
		}

		copyLength := code - 1
		if copyLength > len(frame)-readIndex || copyLength > maxPacketSize-len(decoded) {
			return nil, false
		}

		decoded = append(decoded, frame[readIndex:readIndex+copyLength]...)
		readIndex += copyLength

		if code != 0xff && readIndex < len(frame) {
			if len(decoded) >= maxPacketSize {
				return nil, false
			}
			decoded = append(decoded, 0)
		}
	}

	return decoded, true
}

func parsePacket(data []byte) (backplanePacket, error) {
	if len(data) < 3 {
		return backplanePacket{}, errors.New("short header")
	}

	packet := backplanePacket{
		address: data[0],
		opcode:  data[1],
		flags:   data[2],
		payload: data[3:],
		raw:     data,
	}

	if minSize, ok := minimumPacketSize(packet.opcode); ok && len(data) < minSize {
		return backplanePacket{}, fmt.Errorf("short %s packet: got %d, want at least %d", opcodeName(packet.opcode), len(data), minSize)
	}

	return packet, nil
}

func minimumPacketSize(opcode uint8) (int, bool) {
	switch opcode {
	case 0x00:
		return 3, true
	case 0x01:
		return 9, true
	case 0x02:
		return 4, true
	case 0x03:
		return 5, true
	default:
		return 3, false
	}
}

func opcodeName(opcode uint8) string {
	switch opcode {
	case 0x00:
		return "inquiry"
	case 0x01:
		return "status"
	case 0x02:
		return "set_enabled"
	case 0x03:
		return "set_current_limit"
	default:
		return fmt.Sprintf("unknown_0x%02x", opcode)
	}
}

func formatPacket(packet backplanePacket) string {
	fields := []string{
		fmt.Sprintf("type=%s", opcodeName(packet.opcode)),
		fmt.Sprintf("address=0x%02x", packet.address),
		fmt.Sprintf("eor=%t", packet.flags&0x01 != 0),
	}

	switch packet.opcode {
	case 0x00:
	case 0x01:
		flags := packet.payload[0]
		fields = append(fields,
			fmt.Sprintf("channel=%s", channelName(flags)),
			fmt.Sprintf("enabled=%t", flags&0x02 != 0),
			fmt.Sprintf("tripped=%t", flags&0x04 != 0),
			fmt.Sprintf("module_detected=%t", flags&0x08 != 0),
			fmt.Sprintf("current_limit_deciamps=%d", packet.payload[1]),
			fmt.Sprintf("current_milliamps=%d", binary.LittleEndian.Uint16(packet.payload[2:4])),
			fmt.Sprintf("peak_current_milliamps=%d", binary.LittleEndian.Uint16(packet.payload[4:6])),
		)
	case 0x02:
		flags := packet.payload[0]
		fields = append(fields,
			fmt.Sprintf("channel=%s", channelName(flags)),
			fmt.Sprintf("enabled=%t", flags&0x02 != 0),
		)
	case 0x03:
		fields = append(fields,
			fmt.Sprintf("channel=%s", channelName(packet.payload[0])),
			fmt.Sprintf("current_limit_deciamps=%d", packet.payload[1]),
		)
	default:
		if len(packet.payload) > 0 {
			fields = append(fields, fmt.Sprintf("payload=%s", hex.EncodeToString(packet.payload)))
		}
	}

	return strings.Join(fields, " ")
}

func channelName(flags uint8) string {
	if flags&0x01 != 0 {
		return "rear"
	}
	return "front"
}
