package process

import (
	"fmt"
	"io"
	"strings"
)

func writeDebug(out io.Writer, data *dataset) error {
	var text strings.Builder
	for _, omission := range data.TimetableOmissions {
		fmt.Fprintf(&text, "Excluded endpoint pair: %s (no through pattern in the weekday timetable)\n", omission)
	}
	stationLabel := func(id int) string {
		s := data.Stations[id]
		zone := "in zone"
		if !s.InZone {
			zone = "out of zone"
		}
		return fmt.Sprintf("[%d] %s (%s)", id, s.Name, zone)
	}
	routeLabel := func(id int) string {
		r := data.Routes[id]
		return fmt.Sprintf("[%d] %s: %s -> %s", id, data.Lines[r.Line].Name,
			data.Stations[r.Origin].Name, data.Stations[r.Destination].Name)
	}
	fmt.Fprintf(&text, "Removed %d routes with no weekday daytime service or an interval above 15 minutes.\n\n", data.RemovedRoutes)
	fmt.Fprintf(&text, "Stations (%d)\n", len(data.Stations))
	for id, s := range data.Stations {
		fmt.Fprintln(&text, stationLabel(id))
		if len(s.Routes) == 0 {
			fmt.Fprintln(&text, "  No routes in retained sequences (terminus metadata only)")
		}
		for _, routeID := range s.Routes {
			fmt.Fprintf(&text, "  Route %s\n", routeLabel(int(routeID)))
		}
	}
	fmt.Fprintf(&text, "\nRoutes (%d) — retained stops in visitation order\n", len(data.Routes))
	for id, r := range data.Routes {
		fmt.Fprintf(&text, "Route %s\n", routeLabel(id))
		if r.HasVia {
			fmt.Fprintf(&text, "  Via %s (through route stop %d)\n", data.Stations[r.Via].Name, r.ViaStopIndex+1)
		}
		if r.Frequency.Schedule != "" {
			fmt.Fprintf(&text, "  Timetable: %s, evaluated over 10:00–16:00 at origin\n", r.Frequency.Schedule)
		}
		switch r.Frequency.Source {
		case frequencyJourneys, frequencyPeriod:
			source := "scheduled departures"
			if r.Frequency.Source == frequencyPeriod {
				source = "published frequency range"
			}
			fmt.Fprintf(&text, "  Weekday daytime average headway: %d–%d seconds (%s)\n", r.Frequency.MinSeconds, r.Frequency.MaxSeconds, source)
			if r.Frequency.Source == frequencyJourneys {
				fmt.Fprintf(&text, "  Scheduled departures 10:00–16:00: %d\n", r.Frequency.DeparturesInDaytimeWindow)
			}
		case frequencyNoService:
			fmt.Fprintf(&text, "  Weekday daytime: no service (%s)\n", r.Frequency.Note)
		default:
			fmt.Fprintf(&text, "  Weekday daytime frequency: unknown (%s)\n", r.Frequency.Note)
		}
		for position, stationID := range r.Stops {
			fmt.Fprintf(&text, "  %d. %s\n", position+1, stationLabel(int(stationID)))
		}
	}
	_, err := io.WriteString(out, text.String())
	return err
}
