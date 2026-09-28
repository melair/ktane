package process

import (
	"fmt"
	"slices"
	"sort"
	"strconv"
	"strings"

	"github.com/melair/ktane/Firmware/Tool/cmd/module/underground/internal/snapshot"
)

// Route sequences provide station metadata and the requested endpoint pairs.
// Timetables provide authoritative stopping patterns for those pairs. Do not
// splice short workings together or borrow another destination's frequency.
func reconcileTimetables(sequences []sequence, responses []snapshot.TimetableResponse) ([]sequence, []string, error) {
	timetables, err := loadTimetables(responses)
	if err != nil {
		return nil, nil, err
	}
	requested := map[[3]string]bool{}
	viaByEndpoint := map[[3]string][]string{}
	names := map[string]string{}
	output := make([]sequence, len(sequences))
	for i, seq := range sequences {
		output[i] = seq
		output[i].Routes = nil
		for _, s := range seq.Stations {
			names[s.ID] = s.Name
		}
		for _, branch := range seq.Sequences {
			for _, s := range branch.Stops {
				names[s.ID] = s.Name
			}
		}
		for _, p := range seq.Routes {
			if p.Service != "Regular" {
				continue
			}
			if len(p.IDs) < 2 {
				return nil, nil, fmt.Errorf("incomplete route on %s", seq.LineID)
			}
			key := [3]string{seq.LineID, p.IDs[0], p.IDs[len(p.IDs)-1]}
			requested[key] = true
			if via := viaStopID(p, names); via != "" {
				viaByEndpoint[key] = append(viaByEndpoint[key], via)
			}
		}
	}
	keys := make([][3]string, 0, len(requested))
	for key := range requested {
		keys = append(keys, key)
	}
	sort.Slice(keys, func(i, j int) bool {
		for k := 0; k < 3; k++ {
			if keys[i][k] != keys[j][k] {
				return keys[i][k] < keys[j][k]
			}
		}
		return false
	})
	var omissions []string
	for _, key := range keys {
		tt, ok := timetables[key]
		if !ok {
			return nil, nil, fmt.Errorf("missing timetable for %s: %s -> %s; run underground fetch again", key[0], key[1], key[2])
		}
		if len(tt.Timetable.Routes) == 0 {
			return nil, nil, fmt.Errorf("empty timetable for %v", key)
		}
		seq := sequence{LineID: key[0]}
		seen := map[string]bool{}
		usableWeekday := false
		for _, r := range tt.Timetable.Routes {
			intervalIDs := map[int]bool{}
			for _, interval := range r.Intervals {
				id, err := strconv.Atoi(interval.ID)
				if err != nil || intervalIDs[id] {
					return nil, nil, fmt.Errorf("invalid/duplicate interval ID %q in %v", interval.ID, key)
				}
				intervalIDs[id] = true
				ids := []string{key[1]}
				for _, s := range interval.Stops {
					if s.ID == "" {
						return nil, nil, fmt.Errorf("empty interval stop ID in %v", key)
					}
					ids = append(ids, s.ID)
				}
				ids = slices.Compact(ids)
				if len(ids) < 2 {
					return nil, nil, fmt.Errorf("incomplete interval %s in %v", interval.ID, key)
				}
				if ids[len(ids)-1] != key[2] {
					continue
				}
				patternKey := strings.Join(ids, "|")
				if seen[patternKey] {
					continue
				}
				seen[patternKey] = true
				via := ""
				for _, candidate := range viaByEndpoint[key] {
					if slices.Contains(ids, candidate) {
						if via != "" && via != candidate {
							return nil, nil, fmt.Errorf("ambiguous via points for %v", key)
						}
						via = candidate
					}
				}
				seq.Routes = append(seq.Routes, pattern{IDs: ids, Service: "Regular", Via: via})
			}
			for _, s := range r.Schedules {
				if weekdayRank(s.Name) < 0 {
					continue
				}
				if len(r.Intervals) > 0 && (s.Journeys != nil || len(s.Periods) > 0) {
					usableWeekday = true
				}
				for _, j := range s.Journeys {
					if !intervalIDs[j.IntervalID] {
						return nil, nil, fmt.Errorf("journey references unknown interval %d in %v", j.IntervalID, key)
					}
				}
			}
		}
		if !usableWeekday {
			return nil, nil, fmt.Errorf("no usable weekday timetable for %v", key)
		}
		if len(seq.Routes) == 0 {
			origin, destination := names[key[1]], names[key[2]]
			if origin == "" {
				origin = key[1]
			}
			if destination == "" {
				destination = key[2]
			}
			omissions = append(omissions, fmt.Sprintf("%s: %s -> %s", key[0], origin, destination))
			continue
		}
		output = append(output, seq)
	}
	return output, omissions, nil
}

// TfL labels branch routes in prose, e.g. "Morden ↔ High Barnet via Bank".
// Resolve it only when the named station is part of that route's own stop list.
func viaStopID(p pattern, names map[string]string) string {
	name := strings.ToLower(p.Name)
	marker := strings.LastIndex(name, " via ")
	if marker < 0 {
		return ""
	}
	via := normalStationName(name[marker+len(" via "):])
	for _, id := range p.IDs {
		if normalStationName(names[id]) == via {
			return id
		}
	}
	return ""
}

func normalStationName(name string) string {
	name = strings.ToLower(strings.TrimSpace(name))
	name = strings.TrimSuffix(name, " underground station")
	name = strings.TrimSuffix(name, " dlr station")
	return strings.Join(strings.Fields(name), " ")
}
