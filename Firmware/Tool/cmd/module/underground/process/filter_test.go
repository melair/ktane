package process

import (
	"reflect"
	"strings"
	"testing"
)

func TestFilterDaytimeRoutesRebuildsReferences(t *testing.T) {
	d := &dataset{
		Lines:    []line{{ID: "unused"}, {ID: "kept"}},
		Stations: []station{{Name: "unused"}, {Name: "origin"}, {Name: "stop"}, {Name: "destination"}},
		Routes: []route{
			{Line: 0, Origin: 0, Destination: 0, Stops: []uint16{0}, Frequency: frequency{Source: frequencyNoService}},
			{Line: 1, Origin: 1, Destination: 3, Stops: []uint16{2, 2}, Frequency: frequency{Source: frequencyUnknown}},
			{Line: 1, Origin: 3, Destination: 1, Stops: []uint16{2}, Frequency: frequency{Source: frequencyJourneys, MinSeconds: 600}},
		},
	}
	if err := filterDaytimeRoutes(d); err != nil {
		t.Fatal(err)
	}
	if d.RemovedRoutes != 1 || len(d.Lines) != 1 || len(d.Stations) != 3 || len(d.Routes) != 2 {
		t.Fatalf("wrong filtered counts: %+v", d)
	}
	for i, r := range d.Routes {
		if r.Line != 0 || !reflect.DeepEqual(r.Stops, [][]uint16{{1, 1}, {1}}[i]) {
			t.Fatalf("bad remapping: %+v", r)
		}
	}
	if d.Routes[0].Origin != 0 || d.Routes[0].Destination != 2 {
		t.Fatal("termini remapping")
	}
	if !reflect.DeepEqual(d.Stations[1].Routes, []uint16{0, 1}) || len(d.Stations[0].Routes) != 0 || len(d.Stations[2].Routes) != 0 {
		t.Fatal("incorrect reverse index")
	}
	if d.Routes[0].Frequency.Source != frequencyUnknown {
		t.Fatal("unknown route discarded")
	}
	var out strings.Builder
	if err := writeDebug(&out, d); err != nil {
		t.Fatal(err)
	}
	if !strings.Contains(out.String(), "Removed 1 routes") {
		t.Fatal("missing removal count")
	}
	first := string(renderHeader(d))
	if err := filterDaytimeRoutes(d); err != nil {
		t.Fatal(err)
	}
	if first != string(renderHeader(d)) {
		t.Fatal("unstable filtered IDs")
	}
}

func TestFilterRejectsEmptyDataset(t *testing.T) {
	d := &dataset{Routes: []route{{Frequency: frequency{Source: frequencyNoService}}}}
	if err := filterDaytimeRoutes(d); err == nil {
		t.Fatal("empty output accepted")
	}
}

func TestFilterMaximumInterval(t *testing.T) {
	d := &dataset{Lines: []line{{ID: "test"}}, Stations: []station{{Name: "A"}, {Name: "B"}}}
	for _, seconds := range []uint32{600, 900, 960, 1800} {
		d.Routes = append(d.Routes, route{Origin: 0, Destination: 1, Stops: []uint16{0, 1}, Frequency: frequency{Source: frequencyJourneys, MinSeconds: seconds, MaxSeconds: seconds}})
	}
	if err := filterDaytimeRoutes(d); err != nil {
		t.Fatal(err)
	}
	if d.RemovedRoutes != 2 || len(d.Routes) != 2 {
		t.Fatalf("wrong filtering: %+v", d)
	}
	if everyMinutes(d.Routes[0].Frequency) != 10 || everyMinutes(d.Routes[1].Frequency) != 15 {
		t.Fatal("15-minute boundary excluded")
	}
	for _, s := range d.Stations {
		if !reflect.DeepEqual(s.Routes, []uint16{0, 1}) {
			t.Fatal("stale route IDs")
		}
	}
}
