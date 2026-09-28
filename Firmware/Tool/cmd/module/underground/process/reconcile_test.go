package process

import (
	"encoding/json"
	"os"
	"path/filepath"
	"reflect"
	"strconv"
	"strings"
	"testing"

	"github.com/melair/ktane/Firmware/Tool/cmd/module/underground/internal/snapshot"
)

// Synthetic IDs deliberately have no London-specific names or special cases.
func fixtureTimetables(t *testing.T, sequences []sequence) []snapshot.TimetableResponse {
	t.Helper()
	var result []snapshot.TimetableResponse
	seen := map[[3]string]bool{}
	for _, seq := range sequences {
		for _, p := range seq.Routes {
			if p.Service != "Regular" {
				continue
			}
			key := [3]string{seq.LineID, p.IDs[0], p.IDs[len(p.IDs)-1]}
			if seen[key] {
				continue
			}
			seen[key] = true
			var stops []map[string]string
			for _, id := range p.IDs[1:] {
				stops = append(stops, map[string]string{"stopId": id})
			}
			raw, err := json.Marshal(map[string]any{"lineId": key[0], "timetable": map[string]any{
				"departureStopId": key[1], "routes": []any{map[string]any{
					"stationIntervals": []any{map[string]any{"id": "0", "intervals": stops}},
					"schedules": []any{map[string]any{"name": "Monday - Friday", "knownJourneys": []any{
						map[string]any{"hour": "10", "minute": "00", "intervalId": 0}, map[string]any{"hour": "11", "minute": "00", "intervalId": 0},
					}}},
				}},
			}})
			if err != nil {
				t.Fatal(err)
			}
			result = append(result, snapshot.TimetableResponse{LineID: key[0], Origin: key[1], Destination: key[2], Data: frequentFixture(t, raw)})
		}
	}
	return result
}

func frequentFixture(t *testing.T, raw []byte) json.RawMessage {
	t.Helper()
	var tt timetable
	if err := json.Unmarshal(raw, &tt); err != nil {
		t.Fatal(err)
	}
	for ri := range tt.Timetable.Routes {
		for si := range tt.Timetable.Routes[ri].Schedules {
			s := &tt.Timetable.Routes[ri].Schedules[si]
			original := s.Journeys
			s.Journeys = nil
			seen := map[int]bool{}
			for _, j := range original {
				minute, err := j.clockTime.minutes()
				if err != nil {
					t.Fatal(err)
				}
				if minute < 600 || minute >= 960 {
					s.Journeys = append(s.Journeys, j)
					continue
				}
				if seen[j.IntervalID] {
					continue
				}
				seen[j.IntervalID] = true
				step := 10
				if j.IntervalID == 1 {
					step = 5
				}
				for minute := 600; minute < 960; minute += step {
					j.Hour, j.Minute = strconv.Itoa(minute/60), strconv.Itoa(minute%60)
					s.Journeys = append(s.Journeys, j)
				}
			}
		}
	}
	result, err := json.Marshal(tt)
	if err != nil {
		t.Fatal(err)
	}
	return result
}

func archiveFor(t *testing.T, seq sequence, responses []snapshot.TimetableResponse) []byte {
	t.Helper()
	lines, _ := json.Marshal([]line{{ID: seq.LineID, Name: "Synthetic line"}})
	rawSeq, _ := json.Marshal(seq)
	raw, err := json.Marshal(snapshot.Data{Version: 1, Lines: lines, Sequences: []snapshot.Response{
		{LineID: seq.LineID, Direction: "inbound", Data: rawSeq}, {LineID: seq.LineID, Direction: "outbound", Data: rawSeq},
	}, Timetables: responses})
	if err != nil {
		t.Fatal(err)
	}
	return raw
}

func TestReconcileUsesTimetableStopsAndKeepsBranchesDistinct(t *testing.T) {
	// The published route includes B. The two timetabled paths instead use B or D.
	seq := sequence{LineID: "test", Stations: []stop{
		{ID: "A", Name: "A", Zone: "3"}, {ID: "B", Name: "B", Zone: "1"},
		{ID: "D", Name: "D", Zone: "1"}, {ID: "C", Name: "C", Zone: "3"},
	}, Routes: []pattern{
		{IDs: []string{"A", "B", "D", "C"}, Service: "Regular", Name: "A to C via B"},
		{IDs: []string{"A", "D", "B", "C"}, Service: "Regular", Name: "A to C via D"},
	}}
	responses := []snapshot.TimetableResponse{{LineID: "test", Origin: "A", Destination: "C", Data: frequentFixture(t, []byte(branchTimetable))}}
	data, err := processSnapshot(archiveFor(t, seq, responses))
	if err != nil {
		t.Fatal(err)
	}
	if len(data.Routes) != 2 {
		t.Fatalf("expected both timetabled paths: %+v", data.Routes)
	}
	for i, r := range data.Routes {
		want := [][]string{{"A", "B", "C"}, {"A", "D", "C"}}[i]
		var got []string
		for _, id := range r.Stops {
			got = append(got, data.Stations[id].Name)
		}
		if !reflect.DeepEqual(got, want) {
			t.Fatalf("stops: %v != %v", got, want)
		}
		if everyMinutes(r.Frequency) != []uint32{10, 5}[i] {
			t.Fatal("branch frequency mixed")
		}
		if !r.HasVia || data.Stations[r.Via].Name != []string{"B", "D"}[i] || r.ViaStopIndex != 1 {
			t.Fatalf("via missing or incorrect: %+v", r)
		}
	}
	// Corrected timetable shapes must not depend on the original branch label,
	// or on the order in which the requested route variants were listed.
	seq.Routes = append(seq.Routes, pattern{IDs: []string{"A", "D", "B", "C"}, Service: "Regular", Name: "A to C via D"})
	again, err := processSnapshot(archiveFor(t, seq, responses))
	if err != nil {
		t.Fatal(err)
	}
	if string(renderHeader(data)) != string(renderHeader(again)) {
		t.Fatal("endpoint requests double counted")
	}
}

func TestViaOnlyUsesAStationOnTheTimetabledPattern(t *testing.T) {
	seq := sequence{LineID: "test", Stations: []stop{{ID: "A", Name: "A", Zone: "1"}, {ID: "B", Name: "B", Zone: "1"}, {ID: "C", Name: "C", Zone: "1"}, {ID: "D", Name: "D", Zone: "1"}}, Routes: []pattern{{IDs: []string{"A", "B", "D", "C"}, Service: "Regular", Name: "A to C via B"}}}
	responses := []snapshot.TimetableResponse{{LineID: "test", Origin: "A", Destination: "C", Data: frequentFixture(t, []byte(branchTimetable))}}
	d, err := processSnapshot(archiveFor(t, seq, responses))
	if err != nil {
		t.Fatal(err)
	}
	// The D branch is a legitimate timetable pattern but has no matching TfL via label.
	for _, r := range d.Routes {
		if r.HasVia && d.Stations[r.Via].Name != "B" {
			t.Fatalf("invented via %s", d.Stations[r.Via].Name)
		}
	}
}

func TestReconcileDoesNotSpliceShortWorkings(t *testing.T) {
	seq := sequence{LineID: "test", Routes: []pattern{{IDs: []string{"A", "B", "C"}, Service: "Regular"}}}
	short := sequence{LineID: "test", Routes: []pattern{{IDs: []string{"A", "B"}, Service: "Regular"}}}
	responses := fixtureTimetables(t, []sequence{short})
	responses[0].Destination = "C"
	out, omissions, err := reconcileTimetables([]sequence{seq}, responses)
	if err != nil {
		t.Fatal(err)
	}
	if len(omissions) != 1 {
		t.Fatal("unsupported endpoint pair not reported")
	}
	for _, s := range out {
		if len(s.Routes) != 0 {
			t.Fatal("invented through route")
		}
	}
}

func TestReconcileDropsNonDaytimeVariants(t *testing.T) {
	seq := sequence{LineID: "test", Stations: []stop{{ID: "A", Name: "A", Zone: "1"}, {ID: "B", Name: "B", Zone: "1"}, {ID: "C", Name: "C", Zone: "1"}, {ID: "D", Name: "D", Zone: "1"}}, Routes: []pattern{{IDs: []string{"A", "B", "C"}, Service: "Regular"}}}
	raw := strings.ReplaceAll(strings.ReplaceAll(branchTimetable, `"hour":"11","minute":"55"`, `"hour":"09","minute":"55"`), `"hour":"12","minute":"05"`, `"hour":"16","minute":"05"`)
	responses := []snapshot.TimetableResponse{{LineID: "test", Origin: "A", Destination: "C", Data: frequentFixture(t, []byte(raw))}}
	d, err := processSnapshot(archiveFor(t, seq, responses))
	if err != nil {
		t.Fatal(err)
	}
	if len(d.Routes) != 1 || d.RemovedRoutes != 1 || !reflect.DeepEqual(d.Routes[0].RawStops, []string{"A", "D", "C"}) {
		t.Fatalf("wrong daytime pattern: %+v", d)
	}
	for _, s := range d.Stations {
		if s.Name == "B" {
			t.Fatal("unused stop remains")
		}
	}
}

func TestReconcileRequiresCompleteTimetables(t *testing.T) {
	seq := sequence{LineID: "test", Stations: []stop{{ID: "A", Name: "A", Zone: "1"}, {ID: "C", Name: "C", Zone: "1"}}, Routes: []pattern{{IDs: []string{"A", "C"}, Service: "Regular"}}}
	responses := fixtureTimetables(t, []sequence{seq})
	for _, tc := range []struct {
		name      string
		responses []snapshot.TimetableResponse
	}{
		{"missing", nil},
		{"no weekday", []snapshot.TimetableResponse{{LineID: "test", Origin: "A", Destination: "C", Data: json.RawMessage(strings.ReplaceAll(string(responses[0].Data), "Monday - Friday", "Sunday"))}}},
		{"empty", []snapshot.TimetableResponse{{LineID: "test", Origin: "A", Destination: "C", Data: json.RawMessage(`{"lineId":"test","timetable":{"departureStopId":"A","routes":[]}}`)}}},
	} {
		t.Run(tc.name, func(t *testing.T) {
			dir := t.TempDir()
			input, output := filepath.Join(dir, "input.json"), filepath.Join(dir, "output.h")
			if err := os.WriteFile(input, archiveFor(t, seq, tc.responses), 0644); err != nil {
				t.Fatal(err)
			}
			if err := os.WriteFile(output, []byte("existing header"), 0644); err != nil {
				t.Fatal(err)
			}
			cmd := NewCommand()
			cmd.SetArgs([]string{"--input", input, "--output", output})
			if err := cmd.Execute(); err == nil {
				t.Fatal("incomplete timetable accepted")
			}
			got, err := os.ReadFile(output)
			if err != nil || string(got) != "existing header" {
				t.Fatal("output overwritten on error")
			}
		})
	}
}
