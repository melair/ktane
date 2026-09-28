package process

import (
	"fmt"
	"strconv"
	"strings"
)

type line struct {
	ID   string `json:"id"`
	Name string `json:"name"`
}
type stop struct {
	ID     string `json:"id"`
	Name   string `json:"name"`
	Zone   string `json:"zone"`
	Parent string `json:"parentId"`
	Top    string `json:"topMostParentId"`
}
type pattern struct {
	IDs     []string `json:"naptanIds"`
	Service string   `json:"serviceType"`
	Name    string   `json:"name"`
	// Via is derived from Name while reconciling a published route with a
	// timetabled stopping pattern. It is intentionally not marshalled: TfL
	// supplies the description in Name, not a separate via-stop field.
	Via string `json:"-"`
}
type sequence struct {
	LineID    string `json:"lineId"`
	Stations  []stop `json:"stations"`
	Sequences []struct {
		Stops []stop `json:"stopPoint"`
	} `json:"stopPointSequences"`
	Routes []pattern `json:"orderedLineRoutes"`
}

func inZone(zone string) (bool, error) {
	parts := strings.FieldsFunc(zone, func(r rune) bool { return r == '/' || r == '+' || r == ',' || r == ' ' })
	if len(parts) == 0 {
		return false, fmt.Errorf("missing zone")
	}
	result := false
	for _, part := range parts {
		n, err := strconv.Atoi(part)
		if err != nil || n < 1 {
			return false, fmt.Errorf("invalid zone %q", zone)
		}
		result = result || n == 1 || n == 2
	}
	return result, nil
}
