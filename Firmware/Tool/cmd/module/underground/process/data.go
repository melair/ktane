package process

import (
	"fmt"
	"sort"
	"strings"
)

type station struct {
	Key, Name string
	InZone    bool
	Routes    []uint16
}
type route struct {
	Line, Origin, Destination uint16
	Stops                     []uint16
	RawStops                  []string
	Via                       uint16
	ViaStopIndex              uint8
	HasVia                    bool
	Frequency                 frequency
}
type dataset struct {
	Lines              []line
	Stations           []station
	Routes             []route
	RemovedRoutes      int
	TimetableOmissions []string
}

const (
	noStation      = ^uint16(0)
	noViaStopIndex = ^uint8(0)
)

func buildDataset(lines []line, sequences []sequence) (*dataset, error) {
	// Resolve explicit station ancestry globally before assigning compact IDs.
	parents := map[string]string{}
	var root func(string) string
	root = func(id string) string {
		p, ok := parents[id]
		if !ok {
			parents[id] = id
			return id
		}
		if p != id {
			parents[id] = root(p)
		}
		return parents[id]
	}
	union := func(a, b string) {
		a, b = root(a), root(b)
		if a != b {
			if a > b {
				a, b = b, a
			}
			parents[b] = a
		}
	}
	var stops []stop
	for _, seq := range sequences {
		stops = append(stops, seq.Stations...)
		for _, branch := range seq.Sequences {
			stops = append(stops, branch.Stops...)
		}
	}
	for _, s := range stops {
		if s.ID == "" {
			return nil, fmt.Errorf("station has no ID")
		}
		root(s.ID)
		for _, p := range []string{s.Parent, s.Top} {
			if p != "" {
				union(s.ID, p)
			}
		}
	}
	known := map[string]station{}
	zoneKnown := map[string]bool{}
	for _, s := range stops {
		key := root(s.ID)
		v := known[key]
		v.Key = key
		name := strings.TrimSpace(s.Name)
		for _, suffix := range []string{" Underground Station", " DLR Station"} {
			name = strings.TrimSuffix(name, suffix)
		}
		if name != "" && (v.Name == "" || name < v.Name) {
			v.Name = name
		}
		if s.Zone != "" {
			inside, err := inZone(s.Zone)
			if err != nil {
				return nil, fmt.Errorf("station %s: %w", s.ID, err)
			}
			if zoneKnown[key] && inside != v.InZone {
				return nil, fmt.Errorf("conflicting zones for station hub %s", key)
			}
			v.InZone = inside
			zoneKnown[key] = true
		}
		known[key] = v
	}
	type candidate struct {
		line        string
		ids         []string
		first, last int
		key         string
		rawStops    []string
		via         string
	}
	var candidates []candidate
	usedStations, usedLines, seen := map[string]bool{}, map[string]bool{}, map[string]bool{}
	for _, seq := range sequences {
		for _, p := range seq.Routes {
			if p.Service != "Regular" {
				continue
			}
			if len(p.IDs) < 2 {
				return nil, fmt.Errorf("incomplete route on %s", seq.LineID)
			}
			ids := make([]string, len(p.IDs))
			first, last := -1, -1
			for i, id := range p.IDs {
				if _, ok := parents[id]; !ok {
					return nil, fmt.Errorf("unresolved station %s on %s", id, seq.LineID)
				}
				key := root(id)
				s := known[key]
				if s.Name == "" || !zoneKnown[key] {
					return nil, fmt.Errorf("missing name/zone for station %s", id)
				}
				ids[i] = key
				if s.InZone {
					if first < 0 {
						first = i
					}
					last = i
				}
			}
			if first < 0 {
				continue
			}
			first = max(0, first-1)
			last = min(len(ids)-1, last+1)
			key := seq.LineID + "|" + strings.Join(ids, "|")
			if seen[key] {
				continue
			}
			seen[key] = true
			candidates = append(candidates, candidate{seq.LineID, ids, first, last, key, p.IDs, p.Via})
			usedLines[seq.LineID] = true
			usedStations[ids[0]] = true
			usedStations[ids[len(ids)-1]] = true
			for _, id := range ids[first : last+1] {
				usedStations[id] = true
			}
		}
	}
	d := &dataset{}
	for _, l := range lines {
		if usedLines[l.ID] {
			d.Lines = append(d.Lines, l)
		}
	}
	sort.Slice(d.Lines, func(i, j int) bool { return d.Lines[i].ID < d.Lines[j].ID })
	for id := range usedStations {
		d.Stations = append(d.Stations, known[id])
	}
	sort.Slice(d.Stations, func(i, j int) bool { return d.Stations[i].Key < d.Stations[j].Key })
	sort.Slice(candidates, func(i, j int) bool { return candidates[i].key < candidates[j].key })
	if len(d.Lines) == 0 || len(candidates) == 0 {
		return nil, fmt.Errorf("no regular routes serving zones 1–2")
	}
	if len(d.Lines) > 65536 || len(d.Stations) > 65536 || len(candidates) > 65536 {
		return nil, fmt.Errorf("dataset exceeds uint16_t ID capacity")
	}
	stationIDs, lineIDs := map[string]uint16{}, map[string]uint16{}
	for i, s := range d.Stations {
		stationIDs[s.Key] = uint16(i)
	}
	for i, l := range d.Lines {
		lineIDs[l.ID] = uint16(i)
	}
	for i, c := range candidates {
		lid, ok := lineIDs[c.line]
		if !ok {
			return nil, fmt.Errorf("unknown line %s", c.line)
		}
		r := route{Line: lid, Origin: stationIDs[c.ids[0]], Destination: stationIDs[c.ids[len(c.ids)-1]], RawStops: c.rawStops, Via: noStation, ViaStopIndex: noViaStopIndex}
		for rawIndex, rawID := range c.rawStops {
			if rawID != c.via || rawIndex < c.first || rawIndex > c.last {
				continue
			}
			if rawIndex-c.first >= 1<<8-1 {
				return nil, fmt.Errorf("via position exceeds uint8_t capacity on %s", c.line)
			}
			r.Via = stationIDs[root(rawID)]
			r.ViaStopIndex = uint8(rawIndex - c.first)
			r.HasVia = true
			break
		}
		served := map[uint16]bool{}
		for _, key := range c.ids[c.first : c.last+1] {
			id := stationIDs[key]
			r.Stops = append(r.Stops, id)
			if !served[id] {
				d.Stations[id].Routes = append(d.Stations[id].Routes, uint16(i))
				served[id] = true
			}
		}
		if len(r.Stops) > 1<<8-1 {
			return nil, fmt.Errorf("route stop count exceeds uint8_t capacity on %s", c.line)
		}
		d.Routes = append(d.Routes, r)
	}
	var stopCount, indexCount uint64
	for _, r := range d.Routes {
		stopCount += uint64(len(r.Stops))
	}
	for _, s := range d.Stations {
		indexCount += uint64(len(s.Routes))
	}
	if stopCount > 1<<16-1 || indexCount > 1<<16-1 {
		return nil, fmt.Errorf("dataset exceeds uint16_t offset capacity")
	}
	for _, s := range d.Stations {
		if len(s.Routes) > 1<<8-1 {
			return nil, fmt.Errorf("station route count exceeds uint8_t capacity")
		}
	}
	return d, nil
}
