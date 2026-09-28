package process

import "fmt"

// Keep existing sorted order while assigning dense IDs to the surviving data.
func filterDaytimeRoutes(data *dataset) error {
	var routes []route
	usedStations, usedLines := make([]bool, len(data.Stations)), make([]bool, len(data.Lines))
	for _, r := range data.Routes {
		if r.Frequency.Source == frequencyNoService || everyMinutes(r.Frequency) > 15 {
			continue
		}
		routes = append(routes, r)
		usedLines[r.Line] = true
		usedStations[r.Origin], usedStations[r.Destination] = true, true
		for _, id := range r.Stops {
			usedStations[id] = true
		}
	}
	if len(routes) == 0 {
		return fmt.Errorf("no routes remain after filtering weekday daytime service (10:00–16:00) and intervals above 15 minutes")
	}
	lineIDs, stationIDs := make([]uint16, len(data.Lines)), make([]uint16, len(data.Stations))
	var lines []line
	var stations []station
	for id, l := range data.Lines {
		if usedLines[id] {
			lineIDs[id] = uint16(len(lines))
			lines = append(lines, l)
		}
	}
	for id, s := range data.Stations {
		if usedStations[id] {
			stationIDs[id] = uint16(len(stations))
			s.Routes = nil
			stations = append(stations, s)
		}
	}
	for rid := range routes {
		r := &routes[rid]
		r.Line, r.Origin, r.Destination = lineIDs[r.Line], stationIDs[r.Origin], stationIDs[r.Destination]
		if r.HasVia {
			r.Via = stationIDs[r.Via]
		}
		stops := make([]uint16, len(r.Stops))
		served := map[uint16]bool{}
		for i, oldID := range r.Stops {
			id := stationIDs[oldID]
			stops[i] = id
			if !served[id] {
				stations[id].Routes = append(stations[id].Routes, uint16(rid))
				served[id] = true
			}
		}
		r.Stops = stops
	}
	data.RemovedRoutes += len(data.Routes) - len(routes)
	data.Lines, data.Stations, data.Routes = lines, stations, routes
	return nil
}
