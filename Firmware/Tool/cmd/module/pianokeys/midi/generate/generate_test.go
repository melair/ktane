package generate

import (
	"bytes"
	"io"
	"os"
	"path/filepath"
	"testing"

	"gitlab.com/gomidi/midi/v2/smf"
)

func TestRunGeneratesAllPianoKeyMIDIFiles(t *testing.T) {
	dir := t.TempDir()

	if err := run(options{outputDir: dir}, io.Discard); err != nil {
		t.Fatalf("run() error = %v", err)
	}

	files, err := os.ReadDir(dir)
	if err != nil {
		t.Fatalf("ReadDir() error = %v", err)
	}
	if len(files) != 36 {
		t.Fatalf("generated file count = %d, want 36", len(files))
	}

	expected := []string{
		"c4_soft_vel24.mid",
		"csharp4_medium_vel64.mid",
		"b4_hard_vel112.mid",
	}
	for _, name := range expected {
		if _, err := os.Stat(filepath.Join(dir, name)); err != nil {
			t.Fatalf("expected generated file %q: %v", name, err)
		}
	}
}

func TestBuildMIDIFileContainsMT32SetupAndSingleNote(t *testing.T) {
	data, err := buildMIDIFile(note{midiNumber: 60}, velocity{value: 24})
	if err != nil {
		t.Fatalf("buildMIDIFile() error = %v", err)
	}

	if !bytes.HasPrefix(data, []byte("MThd\x00\x00\x00\x06\x00\x00\x00\x01\x01\xe0MTrk")) {
		t.Fatalf("MIDI file does not start with expected single-track SMF header")
	}

	events := []struct {
		name string
		data []byte
	}{
		{name: "tempo", data: []byte{0x00, 0xFF, 0x51, 0x03, 0x07, 0xA1, 0x20}},
		{name: "MT-32 reset", data: []byte{0x00, 0xF0, 0x0A, 0x41, 0x10, 0x16, 0x12, 0x7F, 0x00, 0x00, 0x01, 0x00, 0xF7}},
		{name: "program change", data: []byte{0x00, 0xC1, 0x00}},
		{name: "note on", data: []byte{0x00, 0x91, 0x3C, 0x18}},
		{name: "note off after 5 seconds", data: []byte{0xA5, 0x40, 0x81, 0x3C, 0x00}},
		{name: "end after 2 second tail", data: []byte{0x8F, 0x00, 0xFF, 0x2F, 0x00}},
	}

	for _, event := range events {
		if !bytes.Contains(data, event.data) {
			t.Fatalf("MIDI file does not contain %s event bytes % x", event.name, event.data)
		}
	}

	parsed, err := smf.ReadFrom(bytes.NewReader(data))
	if err != nil {
		t.Fatalf("smf.ReadFrom() error = %v", err)
	}
	if parsed.NumTracks() != 1 {
		t.Fatalf("parsed track count = %d, want 1", parsed.NumTracks())
	}
	if got := parsed.TimeFormat.(smf.MetricTicks).Resolution(); got != ticksPerQuarter {
		t.Fatalf("parsed ticks per quarter = %d, want %d", got, ticksPerQuarter)
	}

	var (
		foundTempo    bool
		foundSysEx    bool
		foundProgram  bool
		foundNoteOn   bool
		foundNoteOff  bool
		foundTrackEnd bool
	)
	for _, event := range parsed.Tracks[0] {
		var bpm float64
		if event.Message.GetMetaTempo(&bpm) && bpm == tempoBPM {
			foundTempo = true
		}

		var sysEx []byte
		if event.Message.GetSysEx(&sysEx) && bytes.Equal(sysEx, mt32AllParametersReset()) {
			foundSysEx = true
		}

		var channel, program uint8
		if event.Message.GetProgramChange(&channel, &program) &&
			channel == mt32PartOneChannel &&
			program == 0 {
			foundProgram = true
		}

		var key, velocity uint8
		if event.Message.GetNoteOn(&channel, &key, &velocity) &&
			event.Delta == 0 &&
			channel == mt32PartOneChannel &&
			key == 60 &&
			velocity == 24 {
			foundNoteOn = true
		}

		if event.Message.GetNoteOff(&channel, &key, &velocity) &&
			event.Delta == smf.MetricTicks(ticksPerQuarter).Ticks(tempoBPM, 5_000_000_000) &&
			channel == mt32PartOneChannel &&
			key == 60 {
			foundNoteOff = true
		}

		if bytes.Equal(event.Message, []byte{0xFF, 0x2F, 0x00}) &&
			event.Delta == smf.MetricTicks(ticksPerQuarter).Ticks(tempoBPM, 2_000_000_000) {
			foundTrackEnd = true
		}
	}

	for name, found := range map[string]bool{
		"tempo":        foundTempo,
		"MT-32 SysEx":  foundSysEx,
		"program":      foundProgram,
		"note on":      foundNoteOn,
		"note off":     foundNoteOff,
		"end of track": foundTrackEnd,
	} {
		if !found {
			t.Fatalf("parsed MIDI file did not contain expected %s event", name)
		}
	}
}
