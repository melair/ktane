package fetch

import (
	"context"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"net/url"
	"strconv"
	"time"

	"github.com/melair/ktane/Firmware/Tool/cmd/module/underground/internal/snapshot"
)

type client struct {
	base     string
	http     *http.Client
	interval time.Duration
	next     time.Time
}

func newClient() *client {
	return &client{base: "https://api.tfl.gov.uk", http: &http.Client{Timeout: 30 * time.Second}, interval: time.Second}
}
func wait(ctx context.Context, duration time.Duration) error {
	timer := time.NewTimer(max(duration, 0))
	defer timer.Stop()
	select {
	case <-ctx.Done():
		return ctx.Err()
	case <-timer.C:
		return nil
	}
}
func (c *client) get(ctx context.Context, path string, target any) error {
	for attempt := 0; attempt < 4; attempt++ {
		if err := wait(ctx, time.Until(c.next)); err != nil {
			return err
		}
		req, err := http.NewRequestWithContext(ctx, http.MethodGet, c.base+path, nil)
		if err != nil {
			return err
		}
		req.Header.Set("User-Agent", "KTANE-Underground-Generator/1.0")
		req.Header.Set("Accept", "application/json")
		c.next = time.Now().Add(c.interval)
		resp, err := c.http.Do(req)
		delay := time.Second * time.Duration(1<<attempt)
		if err == nil {
			body, readErr := io.ReadAll(io.LimitReader(resp.Body, 32<<20))
			resp.Body.Close()
			if resp.StatusCode == http.StatusOK && readErr == nil {
				if err := json.Unmarshal(body, target); err != nil {
					return fmt.Errorf("decode %s: %w", path, err)
				}
				return nil
			}
			err = fmt.Errorf("HTTP %s", resp.Status)
			if readErr != nil {
				err = readErr
			}
			if readErr == nil && resp.StatusCode != 429 && resp.StatusCode < 500 {
				return fmt.Errorf("fetch %s: %w", path, err)
			}
			if seconds, e := strconv.Atoi(resp.Header.Get("Retry-After")); e == nil {
				delay = max(delay, time.Duration(seconds)*time.Second)
			} else if date, e := http.ParseTime(resp.Header.Get("Retry-After")); e == nil {
				delay = max(delay, time.Until(date))
			}
		}
		if attempt == 3 {
			return fmt.Errorf("fetch %s after 4 attempts: %w", path, err)
		}
		c.next = time.Now().Add(max(delay, c.interval))
	}
	return nil
}

// Fetch only interprets line IDs and route endpoints to discover requests. Response
// payloads remain intact, including fields not used by the processor.
func fetchSnapshot(ctx context.Context, c *client) (*snapshot.Data, error) {
	result := &snapshot.Data{Version: 1}
	if err := c.get(ctx, "/Line/Mode/tube,dlr", &result.Lines); err != nil {
		return nil, err
	}
	var lines []struct {
		ID string `json:"id"`
	}
	if err := json.Unmarshal(result.Lines, &lines); err != nil {
		return nil, err
	}
	if len(lines) == 0 {
		return nil, fmt.Errorf("TfL returned no Tube/DLR lines")
	}
	for _, l := range lines {
		if l.ID == "" {
			return nil, fmt.Errorf("line has no ID for route discovery")
		}
		for _, direction := range []string{"inbound", "outbound"} {
			path := "/Line/" + url.PathEscape(l.ID) + "/Route/Sequence/" + direction + "?serviceTypes=Regular"
			entry := snapshot.Response{LineID: l.ID, Direction: direction}
			if err := c.get(ctx, path, &entry.Data); err != nil {
				return nil, err
			}
			result.Sequences = append(result.Sequences, entry)
		}
	}
	seen := map[[3]string]bool{}
	for _, entry := range result.Sequences {
		var sequence struct {
			Routes []struct {
				IDs []string `json:"naptanIds"`
			} `json:"orderedLineRoutes"`
		}
		if err := json.Unmarshal(entry.Data, &sequence); err != nil {
			return nil, err
		}
		for _, route := range sequence.Routes {
			if len(route.IDs) < 2 {
				continue
			}
			key := [3]string{entry.LineID, route.IDs[0], route.IDs[len(route.IDs)-1]}
			if seen[key] {
				continue
			}
			seen[key] = true
			timetable := snapshot.TimetableResponse{LineID: key[0], Origin: key[1], Destination: key[2]}
			path := "/Line/" + url.PathEscape(key[0]) + "/Timetable/" + url.PathEscape(key[1]) + "/to/" + url.PathEscape(key[2])
			if err := c.get(ctx, path, &timetable.Data); err != nil {
				return nil, err
			}
			result.Timetables = append(result.Timetables, timetable)
		}
	}
	return result, nil
}
