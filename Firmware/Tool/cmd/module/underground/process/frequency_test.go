package process

import (
	"encoding/json"
	"strings"
	"testing"

	"github.com/melair/ktane/Firmware/Tool/cmd/module/underground/internal/snapshot"
)

func timetableFixture(t *testing.T, body string) timetable {
	t.Helper()
	var tt timetable
	if err := json.Unmarshal([]byte(body), &tt); err != nil {
		t.Fatal(err)
	}
	return tt
}

const branchTimetable = `{"lineId":"test","timetable":{"departureStopId":"A","routes":[{
 "stationIntervals":[{"id":"0","intervals":[{"stopId":"B"},{"stopId":"C"}]},{"id":"1","intervals":[{"stopId":"D"},{"stopId":"C"}]}],
 "schedules":[{"name":"Monday - Friday","knownJourneys":[
 {"hour":"11","minute":"55","intervalId":0},{"hour":"12","minute":"05","intervalId":0},
 {"hour":"11","minute":"59","intervalId":1},{"hour":"12","minute":"01","intervalId":1},{"hour":"14","minute":"37","intervalId":1}],
 "periods":[{"type":"FrequencyMinutes","fromTime":{"hour":"10","minute":"00"},"toTime":{"hour":"16","minute":"59"},"frequency":{"lowestFrequency":6,"highestFrequency":2}}]},
 {"name":"Sunday","knownJourneys":[{"hour":"11","minute":"30","intervalId":0},{"hour":"12","minute":"30","intervalId":0}]}]}]}}`

func TestFrequencySeparatesBranchesAndWeekends(t *testing.T) {
	tt := timetableFixture(t, branchTimetable)
	for _, tc := range []struct {
		stops   []string
		seconds uint32
	}{{[]string{"A", "B", "C"}, 10800}, {[]string{"A", "D", "C"}, 7200}} {
		f, err := daytimeFrequency(tt, tc.stops)
		if err != nil || f.Source != frequencyJourneys || f.MinSeconds != tc.seconds || f.MaxSeconds != tc.seconds {
			t.Fatalf("%v: %+v, %v", tc.stops, f, err)
		}
	}
	f, err := daytimeFrequency(tt, []string{"A", "X", "C"})
	if err != nil || f.Source != frequencyUnknown {
		t.Fatalf("unmatched pattern: %+v %v", f, err)
	}
	// The overall published range must not be assigned to an individual branch.
	tt.Timetable.Routes[0].Schedules[0].Journeys = nil
	f, err = daytimeFrequency(tt, []string{"A", "B", "C"})
	if err != nil || f.Source != frequencyUnknown {
		t.Fatalf("aggregate range applied: %+v %v", f, err)
	}
}

func TestFrequencyPeriodUnitsAndBounds(t *testing.T) {
	for _, tc := range []struct {
		kind   string
		lo, hi uint32
	}{{"FrequencyMinutes", 120, 360}, {"FrequencyHours", 7200, 21600}} {
		tt := timetableFixture(t, strings.ReplaceAll(branchTimetable, "FrequencyMinutes", tc.kind))
		tt.Timetable.Routes[0].Intervals = tt.Timetable.Routes[0].Intervals[:1]
		tt.Timetable.Routes[0].Schedules[0].Journeys = nil
		f, err := daytimeFrequency(tt, []string{"A", "B", "C"})
		if err != nil || f.Source != frequencyPeriod || f.MinSeconds != tc.lo || f.MaxSeconds != tc.hi {
			t.Fatalf("%s: %+v %v", tc.kind, f, err)
		}
		// Empty arrays are also used for frequency-only schedules.
		tt.Timetable.Routes[0].Schedules[0].Journeys = tt.Timetable.Routes[0].Schedules[1].Journeys[:0]
		f, err = daytimeFrequency(tt, []string{"A", "B", "C"})
		if err != nil || f.Source != frequencyPeriod {
			t.Fatalf("empty journey list: %+v %v", f, err)
		}
		tt.Timetable.Routes[0].Schedules[0].Periods[0].To.Hour = "11"
		f, err = daytimeFrequency(tt, []string{"A", "B", "C"})
		if err != nil || f.Source != frequencyUnknown {
			t.Fatalf("out-of-time range used: %+v %v", f, err)
		}
	}
}

func TestFrequencyWindowBoundariesAndNoService(t *testing.T) {
	tt := timetableFixture(t, branchTimetable)
	tt.Timetable.Routes[0].Schedules[0].Journeys[0].Hour = "10"
	tt.Timetable.Routes[0].Schedules[0].Journeys[0].Minute = "00"
	f, err := daytimeFrequency(tt, []string{"A", "B", "C"})
	if err != nil || f.MinSeconds != 10800 {
		t.Fatalf("window-start departure: %+v %v", f, err)
	}
	tt.Timetable.Routes[0].Schedules[0].Journeys[0].Hour = "09"
	tt.Timetable.Routes[0].Schedules[0].Journeys[1].Hour = "16"
	f, err = daytimeFrequency(tt, []string{"A", "B", "C"})
	if err != nil || f.Source != frequencyNoService {
		t.Fatalf("no service: %+v %v", f, err)
	}
	tt.Timetable.Routes[0].Schedules[0].Name = "Saturday"
	f, err = daytimeFrequency(tt, []string{"A", "B", "C"})
	if err != nil || f.Source != frequencyUnknown {
		t.Fatalf("weekend used: %+v %v", f, err)
	}
}

func TestTimetableDuplicateStopsAndWeekdayFallback(t *testing.T) {
	duplicate := strings.Replace(branchTimetable, `{"stopId":"B"},{"stopId":"C"}`, `{"stopId":"B"},{"stopId":"B"},{"stopId":"C"}`, 1)
	tt := timetableFixture(t, strings.ReplaceAll(duplicate, "Monday - Friday", "Friday"))
	f, err := daytimeFrequency(tt, []string{"A", "B", "C"})
	if err != nil || f.Source != frequencyJourneys || f.MinSeconds != 10800 || f.Schedule != "Friday" || f.DeparturesInDaytimeWindow != 2 {
		t.Fatalf("duplicate stops/weekday fallback: %+v %v", f, err)
	}
	for _, name := range []string{"Saturday (also Good Friday)", "Saturdays and Public Holidays", "Sunday"} {
		if weekdayRank(name) >= 0 {
			t.Fatalf("weekend selected: %s", name)
		}
	}
}

func TestFrequencyAttachesToRouteAndHeader(t *testing.T) {
	d := &dataset{Lines: []line{{ID: "test", Name: "Test"}}, Stations: []station{{Name: "A"}, {Name: "C"}}, Routes: []route{{RawStops: []string{"A", "B", "C"}, Destination: 1}}}
	response := snapshot.TimetableResponse{LineID: "test", Origin: "A", Destination: "C", Data: json.RawMessage(branchTimetable)}
	if err := addFrequencies(d, []snapshot.TimetableResponse{response}); err != nil {
		t.Fatal(err)
	}
	if d.Routes[0].Frequency.MinSeconds != 10800 {
		t.Fatal("frequency not attached")
	}
	if everyMinutes(d.Routes[0].Frequency) != 180 {
		t.Fatal("frequency not converted to minutes")
	}
	var debug strings.Builder
	if err := writeDebug(&debug, d); err != nil {
		t.Fatal(err)
	}
	if !strings.Contains(debug.String(), "10800–10800 seconds") {
		t.Fatal("frequency missing in debug")
	}
	if err := addFrequencies(d, nil); err != nil || d.Routes[0].Frequency.Source != frequencyUnknown {
		t.Fatal("old snapshot compatibility")
	}
}

func TestDaytimeSparseServiceAndExclusiveEnd(t *testing.T) {
	tt := timetableFixture(t, branchTimetable)
	journeys := tt.Timetable.Routes[0].Schedules[0].Journeys
	journeys[0].Hour, journeys[0].Minute = "10", "00"
	journeys[1].Hour, journeys[1].Minute = "16", "00"
	f, err := daytimeFrequency(tt, []string{"A", "B", "C"})
	if err != nil || f.DeparturesInDaytimeWindow != 1 || f.MinSeconds != 21600 || f.Source != frequencyJourneys {
		t.Fatalf("sparse/boundary: %+v %v", f, err)
	}
	journeys[1].Hour, journeys[1].Minute = "15", "59"
	f, err = daytimeFrequency(tt, []string{"A", "B", "C"})
	if err != nil || f.DeparturesInDaytimeWindow != 2 || f.MinSeconds != 10800 {
		t.Fatalf("end boundary: %+v %v", f, err)
	}
}

func TestDurationWeightedPeriods(t *testing.T) {
	tt := timetableFixture(t, branchTimetable)
	r := &tt.Timetable.Routes[0]
	r.Intervals = r.Intervals[:1]
	s := &r.Schedules[0]
	s.Journeys = nil
	p := s.Periods[0]
	p.From = clockTime{Hour: "10", Minute: "00"}
	p.To = clockTime{Hour: "11", Minute: "59"}
	p.Frequency.Low, p.Frequency.High = 10, 10
	s.Periods[0] = p
	p.From = clockTime{Hour: "12", Minute: "00"}
	p.To = clockTime{Hour: "15", Minute: "59"}
	p.Frequency.Low, p.Frequency.High = 20, 20
	s.Periods = append(s.Periods, p)
	// 12 departures in two hours plus 12 in four hours => 900 seconds.
	f, err := daytimeFrequency(tt, []string{"A", "B", "C"})
	if err != nil || f.Source != frequencyPeriod || f.MinSeconds != 900 || f.MaxSeconds != 900 {
		t.Fatalf("weighted: %+v %v", f, err)
	}
	for _, hour := range []string{"11", "13"} {
		s.Periods[1].From.Hour = hour
		f, err = daytimeFrequency(tt, []string{"A", "B", "C"})
		if err != nil || f.Source != frequencyUnknown {
			t.Fatalf("gap/overlap accepted: %+v %v", f, err)
		}
	}
}
