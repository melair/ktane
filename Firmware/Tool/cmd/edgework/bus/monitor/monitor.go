package monitor

import (
	"bufio"
	"context"
	"encoding/hex"
	"errors"
	"fmt"
	"io"
	"os"
	"os/signal"
	"strconv"
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
}

type cobsDecoder struct {
	synced bool
	frame  []byte
}

type edgeworkPacket struct {
	address uint8
	opcode  uint8
	flags   uint8
	payload []byte
	raw     []byte
}

type monitorLine struct {
	timestamp time.Time
	text      string
}

func NewCommand() *cobra.Command {
	opts := options{baudRate: defaultBaudRate}

	cmd := &cobra.Command{
		Use:   "monitor",
		Short: "Monitor COBS-encoded edgework bus traffic",
		RunE: func(cmd *cobra.Command, args []string) error {
			return run(cmd.Context(), opts, cmd.OutOrStdout())
		},
	}

	cmd.Flags().StringVar(&opts.port, "port", "", "Serial port path")
	cmd.Flags().IntVar(&opts.baudRate, "baud", opts.baudRate, "Serial baud rate")
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

	fmt.Fprintf(out, "monitoring %s at %d baud\n", opts.port, opts.baudRate)
	fmt.Fprintln(out, "commands: inject, help, quit")

	lines := make(chan monitorLine, 64)
	readErrors := make(chan error, 1)
	input := make(chan string)
	go readSerial(ctx, port, lines, readErrors)
	go readInput(os.Stdin, input)

	for {
		select {
		case <-ctx.Done():
			return nil
		case err := <-readErrors:
			return err
		case line := <-lines:
			printMonitorLine(out, line)
		case command, ok := <-input:
			if !ok {
				return nil
			}
			command = strings.TrimSpace(strings.ToLower(command))
			switch command {
			case "":
			case "h", "help", "?":
				printHelp(out)
			case "i", "inject":
				buffered, err := injectPacket(ctx, port, input, lines, readErrors, out)
				if err != nil {
					return err
				}
				flushBufferedLines(out, buffered)
			case "q", "quit", "exit":
				return nil
			default:
				fmt.Fprintf(out, "unknown command %q; type help\n", command)
			}
		}
	}
}

func readSerial(ctx context.Context, port serial.Port, lines chan<- monitorLine, errors chan<- error) {
	decoder := cobsDecoder{frame: make([]byte, 0, maxEncodedSize)}
	buffer := make([]byte, 256)

	for {
		select {
		case <-ctx.Done():
			return
		default:
		}

		n, err := port.Read(buffer)
		if err != nil {
			errors <- fmt.Errorf("read serial port: %w", err)
			return
		}
		if n == 0 {
			continue
		}

		packets := decoder.service(buffer[:n])
		for _, data := range packets {
			packet, err := parsePacket(data)
			now := time.Now()
			if err != nil {
				lines <- monitorLine{
					timestamp: now,
					text:      fmt.Sprintf("invalid len=%d raw=%s error=%v", len(data), hex.EncodeToString(data), err),
				}
				continue
			}

			lines <- monitorLine{
				timestamp: now,
				text:      fmt.Sprintf("%s raw=%s", formatPacket(packet), hex.EncodeToString(packet.raw)),
			}
		}
	}
}

func readInput(in io.Reader, input chan<- string) {
	scanner := bufio.NewScanner(in)
	for scanner.Scan() {
		input <- scanner.Text()
	}
	close(input)
}

func printHelp(out io.Writer) {
	fmt.Fprintln(out, "commands:")
	fmt.Fprintln(out, "  inject  build and send a packet; captured output is buffered while prompting")
	fmt.Fprintln(out, "  quit    close the monitor")
}

func printMonitorLine(out io.Writer, line monitorLine) {
	fmt.Fprintf(out, "%s %s\n", line.timestamp.Format("2006-01-02 15:04:05.000"), line.text)
}

func flushBufferedLines(out io.Writer, buffered []monitorLine) {
	if len(buffered) == 0 {
		return
	}

	fmt.Fprintf(out, "resuming capture; %d buffered packet(s)\n", len(buffered))
	for _, line := range buffered {
		printMonitorLine(out, line)
	}
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

func encodeCOBS(data []byte) ([]byte, error) {
	if len(data) > maxPacketSize {
		return nil, fmt.Errorf("packet is %d bytes, maximum is %d", len(data), maxPacketSize)
	}

	frame := make([]byte, 0, len(data)+(len(data)/254)+2)
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
	return append(frame, 0), nil
}

func parsePacket(data []byte) (edgeworkPacket, error) {
	if len(data) < 3 {
		return edgeworkPacket{}, errors.New("short header")
	}

	packet := edgeworkPacket{
		address: data[0],
		opcode:  data[1],
		flags:   data[2],
		payload: data[3:],
		raw:     data,
	}

	if minSize, ok := minimumPacketSize(packet.opcode); ok && len(data) < minSize {
		return edgeworkPacket{}, fmt.Errorf("short %s packet: got %d, want at least %d", opcodeName(packet.opcode), len(data), minSize)
	}

	return packet, nil
}

func minimumPacketSize(opcode uint8) (int, bool) {
	switch opcode {
	case 0x00, 0x03:
		return 3, true
	case 0x01:
		return 14, true
	case 0x02:
		return 12, true
	case 0xf0, 0xf1:
		return 4, true
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
		return "display"
	case 0x03:
		return "clear"
	case 0xf0:
		return "set_mode"
	case 0xf1:
		return "set_slot_address"
	default:
		return fmt.Sprintf("unknown_0x%02x", opcode)
	}
}

func formatPacket(packet edgeworkPacket) string {
	fields := []string{
		fmt.Sprintf("type=%s", opcodeName(packet.opcode)),
		fmt.Sprintf("address=0x%02x", packet.address),
		fmt.Sprintf("eor=%t", packet.flags&0x01 != 0),
	}

	switch packet.opcode {
	case 0x01:
		mode := packet.payload[0]
		fields = append(fields,
			fmt.Sprintf("mode=%s", modeName(mode)),
			fmt.Sprintf("state=%s", modeStateName(packet.payload[10])),
			fmt.Sprintf("data={%s}", formatEdgeworkStateForMode(mode, packet.payload[1:10])),
		)
	case 0x02:
		fields = append(fields,
			fmt.Sprintf("data={%s}", formatEdgeworkState(packet.payload[:9])),
		)
	case 0x03:
	case 0xf0:
		fields = append(fields, fmt.Sprintf("new_mode=%s", modeName(packet.payload[0])))
	case 0xf1:
		fields = append(fields, fmt.Sprintf("new_address=0x%02x", packet.payload[0]))
	default:
		if len(packet.payload) > 0 {
			fields = append(fields, fmt.Sprintf("payload=%s", hex.EncodeToString(packet.payload)))
		}
	}

	return strings.Join(fields, " ")
}

func formatEdgeworkStateForMode(mode uint8, data []byte) string {
	if len(data) < 9 {
		return fmt.Sprintf("short:%s", hex.EncodeToString(data))
	}

	parts := []string{formatModeState(mode, data)}
	parts = append(parts, fmt.Sprintf("identify=%t", data[8]&0x01 != 0))
	return strings.Join(parts, " ")
}

func formatModeState(mode uint8, data []byte) string {
	switch mode {
	case 0x00:
		return fmt.Sprintf("serial=%q", printableASCII(data[:6]))
	case 0x01:
		return fmt.Sprintf("indicator=%d indicator_lit=%t", data[0], data[1]&0x01 != 0)
	case 0x02:
		return fmt.Sprintf("batteries=%d", data[0])
	case 0x03:
		return fmt.Sprintf("ports=[%s]", formatPorts(data[0]))
	case 0x04:
		return fmt.Sprintf("2fa=%q 2fa_icons=[%s] 2fa_active_display=%t 2fa_flashing=%t",
			printableASCII(data[:6]),
			formatTwoFAIcons(data[6]),
			data[7]&0x01 != 0,
			data[7]&0x02 != 0,
		)
	case 0xfe:
		return fmt.Sprintf("controller_powered=%t", data[0]&0x01 != 0)
	default:
		return fmt.Sprintf("state=%s", hex.EncodeToString(data))
	}
}

func modeStateName(state uint8) string {
	switch state {
	case 0x00:
		return "init(0x00)"
	case 0x01:
		return "startup(0x01)"
	case 0x02:
		return "idle(0x02)"
	case 0x03:
		return "clear(0x03)"
	case 0x04:
		return "display(0x04)"
	default:
		return fmt.Sprintf("0x%02x", state)
	}
}

func modeName(mode uint8) string {
	switch mode {
	case 0x00:
		return "serial(0x00)"
	case 0x01:
		return "indicator(0x01)"
	case 0x02:
		return "battery(0x02)"
	case 0x03:
		return "ports(0x03)"
	case 0x04:
		return "2fa(0x04)"
	case 0xfe:
		return "controller(0xfe)"
	case 0xff:
		return "unknown(0xff)"
	default:
		return fmt.Sprintf("0x%02x", mode)
	}
}

func formatEdgeworkState(data []byte) string {
	if len(data) < 9 {
		return fmt.Sprintf("short:%s", hex.EncodeToString(data))
	}

	parts := []string{
		fmt.Sprintf("serial=%q", printableASCII(data[:6])),
		fmt.Sprintf("indicator=%d", data[0]),
		fmt.Sprintf("indicator_lit=%t", data[1]&0x01 != 0),
		fmt.Sprintf("batteries=%d", data[0]),
		fmt.Sprintf("ports=[%s]", formatPorts(data[0])),
		fmt.Sprintf("2fa=%q", printableASCII(data[:6])),
		fmt.Sprintf("2fa_icons=[%s]", formatTwoFAIcons(data[6])),
		fmt.Sprintf("2fa_active_display=%t", data[7]&0x01 != 0),
		fmt.Sprintf("2fa_flashing=%t", data[7]&0x02 != 0),
		fmt.Sprintf("controller_powered=%t", data[0]&0x01 != 0),
		fmt.Sprintf("identify=%t", data[8]&0x01 != 0),
	}

	return strings.Join(parts, " ")
}

func printableASCII(data []byte) string {
	var builder strings.Builder
	for _, value := range data {
		if value >= 0x20 && value <= 0x7e {
			builder.WriteByte(value)
		} else {
			builder.WriteByte('.')
		}
	}
	return builder.String()
}

func formatPorts(flags uint8) string {
	names := []struct {
		flag uint8
		name string
	}{
		{0x01, "parallel"},
		{0x02, "serial"},
		{0x04, "dvi"},
		{0x08, "ps2"},
		{0x10, "rj45"},
		{0x20, "stereo_rca"},
	}

	var enabled []string
	for _, port := range names {
		if flags&port.flag != 0 {
			enabled = append(enabled, port.name)
		}
	}
	if len(enabled) == 0 {
		return "none"
	}
	return strings.Join(enabled, ",")
}

func formatTwoFAIcons(flags uint8) string {
	names := []struct {
		flag uint8
		name string
	}{
		{0x01, "t1"},
		{0x02, "t2"},
		{0x04, "t3"},
		{0x08, "t4"},
		{0x10, "col1"},
		{0x20, "dot"},
	}

	var enabled []string
	for _, icon := range names {
		if flags&icon.flag != 0 {
			enabled = append(enabled, icon.name)
		}
	}
	if len(enabled) == 0 {
		return "none"
	}
	return strings.Join(enabled, ",")
}

func injectPacket(ctx context.Context, port serial.Port, input <-chan string, lines <-chan monitorLine, readErrors <-chan error, out io.Writer) ([]monitorLine, error) {
	var buffered []monitorLine

	fmt.Fprintln(out, "capture display paused; serial packets will be buffered")
	packet, ok, err := promptPacket(ctx, input, lines, readErrors, out, &buffered)
	if err != nil {
		return buffered, err
	}
	if !ok {
		fmt.Fprintln(out, "injection cancelled")
		return buffered, nil
	}

	frame, err := encodeCOBS(packet)
	if err != nil {
		return buffered, err
	}
	if err := writeAll(port, frame); err != nil {
		return buffered, fmt.Errorf("write serial port: %w", err)
	}
	if err := port.Drain(); err != nil {
		return buffered, fmt.Errorf("drain serial port: %w", err)
	}

	parsed, _ := parsePacket(packet)
	fmt.Fprintf(out, "sent %s raw=%s frame=%s\n", formatPacket(parsed), hex.EncodeToString(packet), hex.EncodeToString(frame))
	return buffered, nil
}

func promptPacket(ctx context.Context, input <-chan string, lines <-chan monitorLine, readErrors <-chan error, out io.Writer, buffered *[]monitorLine) ([]byte, bool, error) {
	var packetType string
	var opcode uint8
	for {
		line, ok, err := promptLine(ctx, input, lines, readErrors, out, buffered, "packet type [inquiry,status,display,clear,set_mode,set_slot_address,raw]: ")
		if err != nil || !ok {
			return nil, ok, err
		}
		packetType = strings.TrimSpace(strings.ToLower(line))
		if isCancel(packetType) {
			return nil, false, nil
		}
		if packetType == "raw" {
			break
		}

		opcode, err = opcodeValue(packetType)
		if err == nil {
			break
		}
		fmt.Fprintln(out, err)
	}

	if packetType == "raw" {
		for {
			raw, ok, err := promptLine(ctx, input, lines, readErrors, out, buffered, "decoded packet hex: ")
			if err != nil || !ok {
				return nil, ok, err
			}
			if isCancel(raw) {
				return nil, false, nil
			}
			packet, err := parseHexBytes(raw)
			if err != nil {
				fmt.Fprintln(out, err)
				continue
			}
			if _, err := parsePacket(packet); err != nil {
				fmt.Fprintln(out, err)
				continue
			}
			return packet, true, nil
		}
	}

	address, ok, err := promptByte(ctx, input, lines, readErrors, out, buffered, "address: ")
	if err != nil || !ok {
		return nil, ok, err
	}
	eor, ok, err := promptBool(ctx, input, lines, readErrors, out, buffered, "eor [y/N]: ", false)
	if err != nil || !ok {
		return nil, ok, err
	}

	packet := []byte{address, opcode, boolByte(eor)}
	switch opcode {
	case 0x00:
	case 0x03:
	case 0x01:
		mode, ok, err := promptByte(ctx, input, lines, readErrors, out, buffered, "mode: ")
		if err != nil || !ok {
			return nil, ok, err
		}
		state, ok, err := promptState(ctx, input, lines, readErrors, out, buffered)
		if err != nil || !ok {
			return nil, ok, err
		}
		stateByte, ok, err := promptModeState(ctx, input, lines, readErrors, out, buffered, "state: ")
		if err != nil || !ok {
			return nil, ok, err
		}
		packet = append(packet, mode)
		packet = append(packet, state...)
		packet = append(packet, stateByte)
	case 0x02:
		mode, ok, err := promptMode(ctx, input, lines, readErrors, out, buffered, "display mode: ")
		if err != nil || !ok {
			return nil, ok, err
		}
		state, ok, err := promptStateForMode(ctx, input, lines, readErrors, out, buffered, mode)
		if err != nil || !ok {
			return nil, ok, err
		}
		packet = append(packet, state...)
	case 0xf0:
		newMode, ok, err := promptByte(ctx, input, lines, readErrors, out, buffered, "new_mode: ")
		if err != nil || !ok {
			return nil, ok, err
		}
		packet = append(packet, newMode)
	case 0xf1:
		newAddress, ok, err := promptByte(ctx, input, lines, readErrors, out, buffered, "new_address: ")
		if err != nil || !ok {
			return nil, ok, err
		}
		packet = append(packet, newAddress)
	}

	return packet, true, nil
}

func promptLine(ctx context.Context, input <-chan string, lines <-chan monitorLine, readErrors <-chan error, out io.Writer, buffered *[]monitorLine, prompt string) (string, bool, error) {
	fmt.Fprint(out, prompt)

	for {
		select {
		case <-ctx.Done():
			return "", false, nil
		case err := <-readErrors:
			return "", false, err
		case line := <-lines:
			*buffered = append(*buffered, line)
		case line, ok := <-input:
			if !ok {
				return "", false, nil
			}
			return line, true, nil
		}
	}
}

func promptByte(ctx context.Context, input <-chan string, lines <-chan monitorLine, readErrors <-chan error, out io.Writer, buffered *[]monitorLine, prompt string) (uint8, bool, error) {
	value, ok, err := promptLine(ctx, input, lines, readErrors, out, buffered, prompt)
	for {
		if err != nil || !ok {
			return 0, ok, err
		}
		if isCancel(value) {
			return 0, false, nil
		}

		parsed, err := strconv.ParseUint(strings.TrimSpace(value), 0, 8)
		if err == nil {
			return uint8(parsed), true, nil
		}

		fmt.Fprintf(out, "parse byte %q: %v\n", value, err)
		value, ok, err = promptLine(ctx, input, lines, readErrors, out, buffered, prompt)
	}
}

func promptBool(ctx context.Context, input <-chan string, lines <-chan monitorLine, readErrors <-chan error, out io.Writer, buffered *[]monitorLine, prompt string, defaultValue bool) (bool, bool, error) {
	value, ok, err := promptLine(ctx, input, lines, readErrors, out, buffered, prompt)
	for {
		if err != nil || !ok {
			return false, ok, err
		}
		switch strings.ToLower(strings.TrimSpace(value)) {
		case "":
			return defaultValue, true, nil
		case "cancel", "c":
			return false, false, nil
		case "y", "yes", "true", "1":
			return true, true, nil
		case "n", "no", "false", "0":
			return false, true, nil
		default:
			fmt.Fprintf(out, "parse bool %q\n", value)
			value, ok, err = promptLine(ctx, input, lines, readErrors, out, buffered, prompt)
		}
	}
}

func promptMode(ctx context.Context, input <-chan string, lines <-chan monitorLine, readErrors <-chan error, out io.Writer, buffered *[]monitorLine, prompt string) (uint8, bool, error) {
	for {
		value, ok, err := promptLine(ctx, input, lines, readErrors, out, buffered, prompt)
		if err != nil || !ok {
			return 0, ok, err
		}
		if isCancel(value) {
			return 0, false, nil
		}

		trimmed := strings.TrimSpace(value)
		if mode, ok := modeValue(strings.ToLower(trimmed)); ok {
			return mode, true, nil
		}

		parsed, err := strconv.ParseUint(trimmed, 0, 8)
		if err == nil {
			return uint8(parsed), true, nil
		}

		fmt.Fprintf(out, "parse mode %q: use serial, indicator, battery, ports, 2fa, or a byte value\n", value)
	}
}

func promptModeState(ctx context.Context, input <-chan string, lines <-chan monitorLine, readErrors <-chan error, out io.Writer, buffered *[]monitorLine, prompt string) (uint8, bool, error) {
	for {
		value, ok, err := promptLine(ctx, input, lines, readErrors, out, buffered, prompt)
		if err != nil || !ok {
			return 0, ok, err
		}
		if isCancel(value) {
			return 0, false, nil
		}

		trimmed := strings.TrimSpace(value)
		if state, ok := modeStateValue(strings.ToLower(trimmed)); ok {
			return state, true, nil
		}

		parsed, err := strconv.ParseUint(trimmed, 0, 8)
		if err == nil {
			return uint8(parsed), true, nil
		}

		fmt.Fprintf(out, "parse state %q: use init, startup, idle, clear, display, or a byte value\n", value)
	}
}

func promptState(ctx context.Context, input <-chan string, lines <-chan monitorLine, readErrors <-chan error, out io.Writer, buffered *[]monitorLine) ([]byte, bool, error) {
	value, ok, err := promptLine(ctx, input, lines, readErrors, out, buffered, "edgework_state hex, 9 bytes: ")
	for {
		if err != nil || !ok {
			return nil, ok, err
		}
		if isCancel(value) {
			return nil, false, nil
		}

		state, err := parseHexBytes(value)
		if err == nil && len(state) == 9 {
			return state, true, nil
		}
		if err != nil {
			fmt.Fprintln(out, err)
		} else {
			fmt.Fprintf(out, "edgework_state must be 9 bytes, got %d\n", len(state))
		}
		value, ok, err = promptLine(ctx, input, lines, readErrors, out, buffered, "edgework_state hex, 9 bytes: ")
	}
}

func promptStateForMode(ctx context.Context, input <-chan string, lines <-chan monitorLine, readErrors <-chan error, out io.Writer, buffered *[]monitorLine, mode uint8) ([]byte, bool, error) {
	state := make([]byte, 9)

	switch mode {
	case 0x00:
		value, ok, err := promptFixedASCII(ctx, input, lines, readErrors, out, buffered, "serial, 6 chars: ", 6)
		if err != nil || !ok {
			return nil, ok, err
		}
		copy(state[:6], value)
	case 0x01:
		indicator, ok, err := promptByte(ctx, input, lines, readErrors, out, buffered, "indicator: ")
		if err != nil || !ok {
			return nil, ok, err
		}
		lit, ok, err := promptBool(ctx, input, lines, readErrors, out, buffered, "lit [y/N]: ", false)
		if err != nil || !ok {
			return nil, ok, err
		}
		state[0] = indicator
		state[1] = boolByte(lit)
	case 0x02:
		count, ok, err := promptByte(ctx, input, lines, readErrors, out, buffered, "battery count: ")
		if err != nil || !ok {
			return nil, ok, err
		}
		state[0] = count
	case 0x03:
		flags, ok, err := promptPorts(ctx, input, lines, readErrors, out, buffered)
		if err != nil || !ok {
			return nil, ok, err
		}
		state[0] = flags
	case 0x04:
		value, ok, err := promptFixedASCII(ctx, input, lines, readErrors, out, buffered, "2fa, 6 chars: ", 6)
		if err != nil || !ok {
			return nil, ok, err
		}
		icons, ok, err := promptTwoFAIcons(ctx, input, lines, readErrors, out, buffered)
		if err != nil || !ok {
			return nil, ok, err
		}
		activeDisplay, ok, err := promptBool(ctx, input, lines, readErrors, out, buffered, "active_display [y/N]: ", false)
		if err != nil || !ok {
			return nil, ok, err
		}
		flashing, ok, err := promptBool(ctx, input, lines, readErrors, out, buffered, "flashing [y/N]: ", false)
		if err != nil || !ok {
			return nil, ok, err
		}
		copy(state[:6], value)
		state[6] = icons
		state[7] = boolByte(activeDisplay) | (boolByte(flashing) << 1)
	case 0xfe:
		powered, ok, err := promptBool(ctx, input, lines, readErrors, out, buffered, "powered [y/N]: ", false)
		if err != nil || !ok {
			return nil, ok, err
		}
		state[0] = boolByte(powered)
	default:
		fmt.Fprintf(out, "no structured prompts for %s; falling back to raw state\n", modeName(mode))
		return promptState(ctx, input, lines, readErrors, out, buffered)
	}

	identify, ok, err := promptBool(ctx, input, lines, readErrors, out, buffered, "identify [y/N]: ", false)
	if err != nil || !ok {
		return nil, ok, err
	}
	state[8] = boolByte(identify)

	return state, true, nil
}

func promptFixedASCII(ctx context.Context, input <-chan string, lines <-chan monitorLine, readErrors <-chan error, out io.Writer, buffered *[]monitorLine, prompt string, length int) ([]byte, bool, error) {
	for {
		value, ok, err := promptLine(ctx, input, lines, readErrors, out, buffered, prompt)
		if err != nil || !ok {
			return nil, ok, err
		}
		if isCancel(value) {
			return nil, false, nil
		}
		if len(value) == length {
			return []byte(value), true, nil
		}
		fmt.Fprintf(out, "value must be exactly %d ASCII characters, got %d\n", length, len(value))
	}
}

func promptPorts(ctx context.Context, input <-chan string, lines <-chan monitorLine, readErrors <-chan error, out io.Writer, buffered *[]monitorLine) (uint8, bool, error) {
	prompts := []struct {
		prompt string
		flag   uint8
	}{
		{"parallel [y/N]: ", 0x01},
		{"serial [y/N]: ", 0x02},
		{"dvi [y/N]: ", 0x04},
		{"ps2 [y/N]: ", 0x08},
		{"rj45 [y/N]: ", 0x10},
		{"stereo_rca [y/N]: ", 0x20},
	}

	var flags uint8
	for _, item := range prompts {
		enabled, ok, err := promptBool(ctx, input, lines, readErrors, out, buffered, item.prompt, false)
		if err != nil || !ok {
			return 0, ok, err
		}
		if enabled {
			flags |= item.flag
		}
	}
	return flags, true, nil
}

func promptTwoFAIcons(ctx context.Context, input <-chan string, lines <-chan monitorLine, readErrors <-chan error, out io.Writer, buffered *[]monitorLine) (uint8, bool, error) {
	prompts := []struct {
		prompt string
		flag   uint8
	}{
		{"icon t1 [y/N]: ", 0x01},
		{"icon t2 [y/N]: ", 0x02},
		{"icon t3 [y/N]: ", 0x04},
		{"icon t4 [y/N]: ", 0x08},
		{"icon col1 [y/N]: ", 0x10},
		{"icon dot [y/N]: ", 0x20},
	}

	var flags uint8
	for _, item := range prompts {
		enabled, ok, err := promptBool(ctx, input, lines, readErrors, out, buffered, item.prompt, false)
		if err != nil || !ok {
			return 0, ok, err
		}
		if enabled {
			flags |= item.flag
		}
	}
	return flags, true, nil
}

func parseHexBytes(value string) ([]byte, error) {
	value = strings.NewReplacer(" ", "", ":", "", "-", "", "_", "").Replace(value)
	if len(value)%2 != 0 {
		return nil, errors.New("hex input must contain an even number of digits")
	}
	decoded, err := hex.DecodeString(value)
	if err != nil {
		return nil, fmt.Errorf("parse hex: %w", err)
	}
	return decoded, nil
}

func modeValue(value string) (uint8, bool) {
	switch value {
	case "serial":
		return 0x00, true
	case "indicator":
		return 0x01, true
	case "battery", "batteries":
		return 0x02, true
	case "ports":
		return 0x03, true
	case "2fa", "twofa":
		return 0x04, true
	case "controller":
		return 0xfe, true
	case "unknown":
		return 0xff, true
	default:
		return 0, false
	}
}

func modeStateValue(value string) (uint8, bool) {
	switch value {
	case "init":
		return 0x00, true
	case "startup":
		return 0x01, true
	case "idle":
		return 0x02, true
	case "clear":
		return 0x03, true
	case "display":
		return 0x04, true
	default:
		return 0, false
	}
}

func opcodeValue(value string) (uint8, error) {
	switch value {
	case "inquiry", "inq":
		return 0x00, nil
	case "status":
		return 0x01, nil
	case "display":
		return 0x02, nil
	case "clear":
		return 0x03, nil
	case "set_mode", "set-mode", "mode":
		return 0xf0, nil
	case "set_slot_address", "set-slot-address", "slot":
		return 0xf1, nil
	default:
		return 0, fmt.Errorf("unknown packet type %q", value)
	}
}

func boolByte(value bool) byte {
	if value {
		return 1
	}
	return 0
}

func isCancel(value string) bool {
	return strings.EqualFold(strings.TrimSpace(value), "cancel") || strings.EqualFold(strings.TrimSpace(value), "c")
}

func writeAll(writer io.Writer, data []byte) error {
	for len(data) > 0 {
		n, err := writer.Write(data)
		if err != nil {
			return err
		}
		if n == 0 {
			return io.ErrShortWrite
		}
		data = data[n:]
	}
	return nil
}
