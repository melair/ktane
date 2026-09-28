// Package snapshot defines the unprocessed TfL response archive.
package snapshot

import "encoding/json"

type Data struct {
	Version    int                 `json:"version"`
	Lines      json.RawMessage     `json:"lines"`
	Sequences  []Response          `json:"sequences"`
	Timetables []TimetableResponse `json:"timetables,omitempty"`
}

type TimetableResponse struct {
	LineID      string          `json:"line_id"`
	Origin      string          `json:"origin"`
	Destination string          `json:"destination"`
	Data        json.RawMessage `json:"data"`
}

type Response struct {
	LineID    string          `json:"line_id"`
	Direction string          `json:"direction"`
	Data      json.RawMessage `json:"data"`
}
