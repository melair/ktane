package process

import (
	"encoding/json"
	"fmt"
	"math"
	"slices"
	"strconv"
	"strings"

	"github.com/melair/ktane/Firmware/Tool/cmd/module/underground/internal/snapshot"
)

const (
	frequencyUnknown = iota
	frequencyJourneys
	frequencyPeriod
	frequencyNoService
)

// Headways are seconds between trains, not trains per hour. Daytime is evaluated
// at the full route's origin, using a published ordinary weekday schedule.
const (
	daytimeStartMinute = 10 * 60
	daytimeEndMinute   = 16 * 60
	daytimeSeconds     = (daytimeEndMinute - daytimeStartMinute) * 60
)

type frequency struct {
	MinSeconds, MaxSeconds    uint32
	Source                    int
	Note                      string
	Schedule                  string
	DeparturesInDaytimeWindow uint32
}
type clockTime struct {
	Hour   string `json:"hour"`
	Minute string `json:"minute"`
}

func (c clockTime) minutes() (int, error) {
	h, e := strconv.Atoi(c.Hour)
	if e != nil || h < 0 || h > 47 {
		return 0, fmt.Errorf("invalid hour %q", c.Hour)
	}
	m, e := strconv.Atoi(c.Minute)
	if e != nil || m < 0 || m > 59 {
		return 0, fmt.Errorf("invalid minute %q", c.Minute)
	}
	return h*60 + m, nil
}

type timetable struct {
	LineID    string `json:"lineId"`
	Timetable struct {
		DepartureStopID string           `json:"departureStopId"`
		Routes          []timetableRoute `json:"routes"`
	} `json:"timetable"`
}
type timetableRoute struct {
	Intervals []struct {
		ID    string `json:"id"`
		Stops []struct {
			ID string `json:"stopId"`
		} `json:"intervals"`
	} `json:"stationIntervals"`
	Schedules []schedule `json:"schedules"`
}
type schedule struct {
	Name     string `json:"name"`
	Journeys []struct {
		clockTime
		IntervalID int `json:"intervalId"`
	} `json:"knownJourneys"`
	Periods []struct {
		Type      string    `json:"type"`
		From      clockTime `json:"fromTime"`
		To        clockTime `json:"toTime"`
		Frequency struct {
			Low  float64 `json:"lowestFrequency"`
			High float64 `json:"highestFrequency"`
		} `json:"frequency"`
	} `json:"periods"`
}

// Prefer a broad weekday schedule, then a midweek schedule. Some TfL lines
// publish only Friday or Monday; retain that schedule name in the output.
func weekdayRank(name string) int {
	name = strings.ToLower(strings.TrimSpace(strings.Split(name, "(")[0]))
	name = strings.ReplaceAll(name, "–", "-")
	name = strings.ReplaceAll(name, " to ", " - ")
	name = strings.Join(strings.Fields(name), " ")
	switch name {
	case "monday - friday", "weekday", "weekdays":
		return 0
	case "monday - thursday", "wednesday", "wednesdays":
		return 1
	case "monday - saturday", "monday - sunday", "daily":
		return 2
	case "monday", "mondays":
		return 3
	case "tuesday", "tuesdays":
		return 4
	case "thursday", "thursdays":
		return 5
	case "friday", "fridays":
		return 6
	}
	return -1
}

func loadTimetables(responses []snapshot.TimetableResponse) (map[[3]string]timetable, error) {
	byKey := map[[3]string]timetable{}
	for _, response := range responses {
		key := [3]string{response.LineID, response.Origin, response.Destination}
		if _, ok := byKey[key]; ok {
			return nil, fmt.Errorf("duplicate timetable %v", key)
		}
		var tt timetable
		if err := json.Unmarshal(response.Data, &tt); err != nil {
			return nil, fmt.Errorf("decode timetable %v: %w", key, err)
		}
		if tt.LineID != response.LineID || tt.LineID == "" {
			return nil, fmt.Errorf("timetable line mismatch %v", key)
		}
		if tt.Timetable.DepartureStopID != response.Origin || response.Origin == "" {
			return nil, fmt.Errorf("timetable origin mismatch %v", key)
		}
		byKey[key] = tt
	}
	return byKey, nil
}

func addFrequencies(data *dataset, responses []snapshot.TimetableResponse) error {
	byKey, err := loadTimetables(responses)
	if err != nil {
		return err
	}
	for i := range data.Routes {
		r := &data.Routes[i]
		key := [3]string{data.Lines[r.Line].ID, r.RawStops[0], r.RawStops[len(r.RawStops)-1]}
		tt, ok := byKey[key]
		if !ok {
			r.Frequency = frequency{Note: "timetable not fetched"}
			continue
		}
		f, err := daytimeFrequency(tt, r.RawStops)
		if err != nil {
			return fmt.Errorf("timetable %v: %w", key, err)
		}
		r.Frequency = f
	}
	return nil
}

func daytimeFrequency(tt timetable, stops []string) (result frequency, err error) {
	var scheduleName string
	var daytimeDepartures uint32
	defer func() { result.Schedule = scheduleName; result.DeparturesInDaytimeWindow = daytimeDepartures }()
	var departures []int
	var periods []frequencySpan
	matched, weekday, hasJourneys := false, false, false
	for _, route := range tt.Timetable.Routes {
		matches := map[int]bool{}
		for _, interval := range route.Intervals {
			ids := []string{stops[0]}
			for _, stop := range interval.Stops {
				ids = append(ids, stop.ID)
			}
			// Timetable intervals can contain consecutive arrival/departure entries
			// for one station. Collapse those only; retain actual loop revisits.
			if !slices.Equal(slices.Compact(ids), slices.Compact(slices.Clone(stops))) {
				continue
			}
			id, err := strconv.Atoi(interval.ID)
			if err != nil {
				return frequency{}, fmt.Errorf("invalid interval ID %q", interval.ID)
			}
			matches[id] = true
		}
		if len(matches) == 0 {
			continue
		}
		matched = true
		var selected []schedule
		bestRank := 100
		for _, s := range route.Schedules {
			rank := weekdayRank(s.Name)
			if rank >= 0 && rank <= bestRank {
				if rank < bestRank {
					selected = nil
					bestRank = rank
				}
				selected = append(selected, s)
			}
		}
		if len(selected) > 1 {
			return frequency{Note: "multiple applicable weekday schedules"}, nil
		}
		if len(selected) == 0 {
			continue
		}
		weekday = true
		s := selected[0]
		if scheduleName != "" && scheduleName != s.Name {
			return frequency{Note: "inconsistent weekday schedules across patterns"}, nil
		}
		scheduleName = s.Name
		// An empty list alongside frequency periods is a frequency-only schedule,
		// not evidence that the service has no departures.
		hasJourneys = hasJourneys || len(s.Journeys) > 0 || (s.Journeys != nil && len(s.Periods) == 0)
		for _, j := range s.Journeys {
			if !matches[j.IntervalID] {
				continue
			}
			minute, err := j.clockTime.minutes()
			if err != nil {
				return frequency{}, err
			}
			departures = append(departures, minute)
		}
		// Period ranges describe all interval patterns together. Only use them when
		// every pattern belongs to this exact route; otherwise branch rates differ.
		if len(matches) != len(route.Intervals) {
			continue
		}
		for _, p := range s.Periods {
			if p.Type != "FrequencyMinutes" && p.Type != "FrequencyHours" {
				continue
			}
			from, err := p.From.minutes()
			if err != nil {
				return frequency{}, err
			}
			to, err := p.To.minutes()
			if err != nil {
				return frequency{}, err
			}
			// TfL's clock periods end on an inclusive minute (e.g. 15:59).
			to++
			if to <= from {
				to += 24 * 60
			}

			unit := 60.0
			if p.Type == "FrequencyHours" {
				unit = 3600
			}
			lo, hi := math.Round(min(p.Frequency.Low, p.Frequency.High)*unit), math.Round(max(p.Frequency.Low, p.Frequency.High)*unit)
			if lo <= 0 || hi > math.MaxUint32 || math.IsNaN(lo) || math.IsNaN(hi) {
				return frequency{}, fmt.Errorf("invalid frequency range")
			}
			for _, shift := range []int{0, -24 * 60} {
				// Only shift periods that cross midnight, not ordinary next-day times.
				if shift != 0 && from < to && to <= 24*60 {
					continue
				}
				left, right := max(from+shift, daytimeStartMinute), min(to+shift, daytimeEndMinute)
				if left < right {
					periods = append(periods, frequencySpan{left, right, lo, hi})
				}
			}
		}
	}
	slices.Sort(departures)
	departures = slices.Compact(departures)
	for _, minute := range departures {
		if minute >= daytimeStartMinute && minute < daytimeEndMinute {
			daytimeDepartures++
		}
	}
	if hasJourneys {
		if daytimeDepartures == 0 {
			return frequency{Source: frequencyNoService, Note: "no scheduled departures from 10:00 to 16:00"}, nil
		}
		seconds := uint32(math.Round(float64(daytimeSeconds) / float64(daytimeDepartures)))
		return frequency{MinSeconds: seconds, MaxSeconds: seconds, Source: frequencyJourneys}, nil
	}
	if !matched {
		return frequency{Note: "no exact stopping-pattern match"}, nil
	}
	if !weekday {
		return frequency{Note: "no applicable weekday schedule"}, nil
	}
	return averagePeriods(periods), nil
}

// frequencySpan uses half-open minute boundaries and headways in seconds.
type frequencySpan struct {
	from, to               int
	minSeconds, maxSeconds float64
}

func averagePeriods(periods []frequencySpan) frequency {
	slices.SortFunc(periods, func(a, b frequencySpan) int {
		if a.from != b.from {
			return a.from - b.from
		}
		return a.to - b.to
	})
	cursor := daytimeStartMinute
	var minDepartures, maxDepartures float64
	for _, p := range periods {
		if p.from != cursor {
			return frequency{Note: "daytime frequency periods have gaps or overlaps"}
		}
		seconds := float64((p.to - p.from) * 60)
		minDepartures += seconds / p.maxSeconds
		maxDepartures += seconds / p.minSeconds
		cursor = p.to
	}
	if cursor != daytimeEndMinute {
		return frequency{Note: "frequency periods do not cover 10:00–16:00"}
	}
	return frequency{
		MinSeconds: uint32(math.Round(float64(daytimeSeconds) / maxDepartures)),
		MaxSeconds: uint32(math.Round(float64(daytimeSeconds) / minDepartures)),
		Source:     frequencyPeriod,
	}
}
