package generate

import (
	"bytes"
	"errors"
	"fmt"
	"io"
	"os"
	"path/filepath"
	"time"

	"github.com/spf13/cobra"
	"gitlab.com/gomidi/midi/v2"
	"gitlab.com/gomidi/midi/v2/smf"
)

const (
	ticksPerQuarter = 480
	tempoBPM        = 120

	mt32PartOneChannel = 1
)

var notes = []note{
	{name: "C4", fileStem: "c4", midiNumber: 60},
	{name: "C#4", fileStem: "csharp4", midiNumber: 61},
	{name: "D4", fileStem: "d4", midiNumber: 62},
	{name: "D#4", fileStem: "dsharp4", midiNumber: 63},
	{name: "E4", fileStem: "e4", midiNumber: 64},
	{name: "F4", fileStem: "f4", midiNumber: 65},
	{name: "F#4", fileStem: "fsharp4", midiNumber: 66},
	{name: "G4", fileStem: "g4", midiNumber: 67},
	{name: "G#4", fileStem: "gsharp4", midiNumber: 68},
	{name: "A4", fileStem: "a4", midiNumber: 69},
	{name: "A#4", fileStem: "asharp4", midiNumber: 70},
	{name: "B4", fileStem: "b4", midiNumber: 71},
}

var velocities = []velocity{
	{name: "soft", value: 24},
	{name: "medium", value: 64},
	{name: "hard", value: 112},
}

type options struct {
	outputDir string
}

type note struct {
	name       string
	fileStem   string
	midiNumber byte
}

type velocity struct {
	name  string
	value byte
}

func NewCommand() *cobra.Command {
	var opts options

	cmd := &cobra.Command{
		Use:   "generate",
		Short: "Generate MT-32 Piano Keys MIDI note samples",
		RunE: func(cmd *cobra.Command, args []string) error {
			return run(opts, cmd.OutOrStdout())
		},
	}

	cmd.Flags().StringVar(&opts.outputDir, "output-dir", "", "Directory to write generated MIDI files")
	_ = cmd.MarkFlagRequired("output-dir")

	return cmd
}

func run(opts options, out io.Writer) error {
	if opts.outputDir == "" {
		return errors.New("--output-dir is required")
	}

	if err := os.MkdirAll(opts.outputDir, 0755); err != nil {
		return fmt.Errorf("create output directory: %w", err)
	}

	count := 0
	for _, n := range notes {
		for _, v := range velocities {
			path := filepath.Join(opts.outputDir, fmt.Sprintf("%s_%s_vel%d.mid", n.fileStem, v.name, v.value))
			data, err := buildMIDIFile(n, v)
			if err != nil {
				return fmt.Errorf("build %s: %w", path, err)
			}
			if err := os.WriteFile(path, data, 0644); err != nil {
				return fmt.Errorf("write %s: %w", path, err)
			}
			count++
		}
	}

	fmt.Fprintf(out, "generated %d MIDI file(s) in %s\n", count, opts.outputDir)
	return nil
}

func buildMIDIFile(n note, v velocity) ([]byte, error) {
	clock := smf.MetricTicks(ticksPerQuarter)
	var track smf.Track

	track.Add(0, smf.MetaTempo(tempoBPM))
	track.Add(0, midi.SysEx(mt32AllParametersReset()))
	track.Add(0, midi.ControlChange(mt32PartOneChannel, 121, 0))
	track.Add(0, midi.ControlChange(mt32PartOneChannel, 7, 100))
	track.Add(0, midi.ControlChange(mt32PartOneChannel, 10, 64))
	track.Add(0, midi.ControlChange(mt32PartOneChannel, 64, 0))
	track.Add(0, midi.ProgramChange(mt32PartOneChannel, 0))
	track.Add(0, midi.NoteOn(mt32PartOneChannel, n.midiNumber, v.value))
	track.Add(clock.Ticks(tempoBPM, 5*time.Second), midi.NoteOff(mt32PartOneChannel, n.midiNumber))
	track.Close(clock.Ticks(tempoBPM, 2*time.Second))

	smfFile := smf.New()
	smfFile.TimeFormat = clock
	if err := smfFile.Add(track); err != nil {
		return nil, err
	}
	var buf bytes.Buffer
	if _, err := smfFile.WriteTo(&buf); err != nil {
		return nil, err
	}

	return buf.Bytes(), nil
}

func mt32AllParametersReset() []byte {
	return []byte{0x41, 0x10, 0x16, 0x12, 0x7F, 0x00, 0x00, 0x01, 0x00}
}
