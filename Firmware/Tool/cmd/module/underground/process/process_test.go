package process

import (
	"encoding/json"
	"os"
	"os/exec"
	"path/filepath"
	"reflect"
	"strings"
	"testing"

	"github.com/melair/ktane/Firmware/Tool/cmd/module/underground/internal/snapshot"
)

func TestProcessSnapshotCommand(t *testing.T) {
	lines, sequences := fixture()
	rawLines, _ := json.Marshal(lines)
	rawSequence, _ := json.Marshal(sequences[0])
	archive := snapshot.Data{Version: 1, Lines: rawLines, Sequences: []snapshot.Response{
		{LineID: "test", Direction: "inbound", Data: rawSequence},
		{LineID: "test", Direction: "outbound", Data: rawSequence},
	}}
	archive.Timetables = fixtureTimetables(t, sequences)
	raw, _ := json.Marshal(archive)
	dir := t.TempDir()
	input, output := filepath.Join(dir, "raw.json"), filepath.Join(dir, "data.h")
	if err := os.WriteFile(input, raw, 0644); err != nil {
		t.Fatal(err)
	}
	cmd := NewCommand()
	cmd.SetArgs([]string{"--input", input, "--output", output})
	if err := cmd.Execute(); err != nil {
		t.Fatal(err)
	}
	want, err := buildDataset(lines, sequences)
	if err != nil {
		t.Fatal(err)
	}
	if err := addFrequencies(want, archive.Timetables); err != nil {
		t.Fatal(err)
	}
	got, err := os.ReadFile(output)
	if err != nil || string(got) != string(renderHeader(want)) {
		t.Fatalf("processing differs: %v", err)
	}
	archive.Sequences = archive.Sequences[:1]
	raw, _ = json.Marshal(archive)
	if _, err := processSnapshot(raw); err == nil {
		t.Fatal("incomplete archive accepted")
	}
	archive.Version = 2
	raw, _ = json.Marshal(archive)
	if _, err := processSnapshot(raw); err == nil {
		t.Fatal("unknown version accepted")
	}
}

func fixture() ([]line, []sequence) {
	stops := []stop{}
	for i, name := range []string{"X", "Y", "A", "B", "C", "D", "E", "F"} {
		zone := "3"
		if i == 2 {
			zone = "1"
		}
		if i == 5 {
			zone = "2/3"
		}
		stops = append(stops, stop{ID: name, Name: name + " Underground Station", Zone: zone})
	}
	return []line{{ID: "test", Name: "Test"}}, []sequence{{LineID: "test", Stations: stops, Routes: []pattern{
		{IDs: []string{"X", "Y", "A", "B", "C", "D", "E", "F"}, Service: "Regular"},
		{IDs: []string{"F", "E", "D", "C", "B", "A", "Y", "X"}, Service: "Regular"},
		{IDs: []string{"A", "D"}, Service: "Regular"},
		{IDs: []string{"X", "Y"}, Service: "Regular"},
		{IDs: []string{"A", "Y"}, Service: "Night"},
	}}}
}
func TestTrimmingAndIndexes(t *testing.T) {
	lines, seq := fixture()
	d, err := buildDataset(lines, seq)
	if err != nil {
		t.Fatal(err)
	}
	if len(d.Routes) != 3 {
		t.Fatalf("routes: %d", len(d.Routes))
	}
	for rid, r := range d.Routes {
		names := []string{}
		for _, id := range r.Stops {
			names = append(names, d.Stations[id].Name)
		}
		origin := d.Stations[r.Origin].Name
		expected := map[string][]string{"X": {"Y", "A", "B", "C", "D", "E"}, "F": {"E", "D", "C", "B", "A", "Y"}, "A": {"A", "D"}}[origin]
		if !reflect.DeepEqual(names, expected) {
			t.Fatalf("%s: %v", origin, names)
		}
		for sid, s := range d.Stations {
			present, indexed := false, false
			for _, id := range r.Stops {
				present = present || int(id) == sid
			}
			for _, id := range s.Routes {
				indexed = indexed || int(id) == rid
			}
			if present != indexed {
				t.Fatalf("reverse index mismatch %s", s.Name)
			}
		}
	}
	for _, s := range d.Stations {
		if s.InZone != (s.Name == "A" || s.Name == "D") {
			t.Fatalf("zone %s", s.Name)
		}
	}
	first := string(renderHeader(d))
	seq[0].Routes[0], seq[0].Routes[1] = seq[0].Routes[1], seq[0].Routes[0]
	d, err = buildDataset(lines, seq)
	if err != nil {
		t.Fatal(err)
	}
	if first != string(renderHeader(d)) {
		t.Fatal("nondeterministic output")
	}
}
func TestHubMergeAndRepeatedVisits(t *testing.T) {
	lines, seq := fixture()
	seq[0].Stations = append(seq[0].Stations, stop{ID: "DLRA", Name: "A DLR Station", Zone: "1", Top: "HUBA"})
	seq[0].Stations[2].Parent = "HUBA"
	seq[0].Routes = []pattern{{IDs: []string{"A", "B", "DLRA", "D"}, Service: "Regular"}}
	d, err := buildDataset(lines, seq)
	if err != nil {
		t.Fatal(err)
	}
	r := d.Routes[0]
	if r.Stops[0] != r.Stops[2] {
		t.Fatal("hub not merged")
	}
	if len(d.Stations[r.Stops[0]].Routes) != 1 {
		t.Fatal("repeated reverse index")
	}
	if r.Stops[0] == r.Stops[1] {
		t.Fatal("distinct stations merged")
	}
}
func TestInvalidMetadata(t *testing.T) {
	for _, kind := range []string{"missing stop", "missing zone", "bad zone"} {
		t.Run(kind, func(t *testing.T) {
			lines, seq := fixture()
			switch kind {
			case "missing stop":
				seq[0].Routes[0].IDs[0] = "unknown"
			case "missing zone":
				seq[0].Stations[0].Zone = ""
			case "bad zone":
				seq[0].Stations[0].Zone = "unknown"
			}
			if _, err := buildDataset(lines, seq); err == nil {
				t.Fatal("expected failure")
			}
		})
	}
}
func TestHeaderCompiles(t *testing.T) {
	cc, err := exec.LookPath("cc")
	if err != nil {
		t.Skip("C compiler unavailable")
	}
	lines, seq := fixture()
	d, err := buildDataset(lines, seq)
	if err != nil {
		t.Fatal(err)
	}
	d.Stations[0].Name = "Quotes \" \\ newline\n café ??/ */"
	d.Lines[0].Name = "Line */\nunsafe"
	path := filepath.Join(t.TempDir(), "data.h")
	if err := writeHeader(path, d); err != nil {
		t.Fatal(err)
	}
	cmd := exec.Command(cc, "-std=c99", "-pedantic-errors", "-x", "c", "-fsyntax-only", "-")
	cmd.Stdin = strings.NewReader("#include " + cString(path) + "\nint main(void) { return underground_stations[0].in_zone; }\n")
	if output, err := cmd.CombinedOutput(); err != nil {
		t.Fatalf("%v: %s", err, output)
	}
	if _, err := os.Stat(path); err != nil {
		t.Fatal(err)
	}
}
