package fetch

import (
	"context"
	"encoding/json"
	"fmt"
	"net/http"
	"net/http/httptest"
	"os"
	"path/filepath"
	"testing"
	"time"
)

func TestHTTPRetriesAndCancellation(t *testing.T) {
	calls := 0
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		calls++
		if calls == 1 {
			w.Header().Set("Retry-After", "1")
			w.WriteHeader(429)
			return
		}
		fmt.Fprint(w, `{"id":"ok"}`)
	}))
	defer server.Close()
	c := &client{base: server.URL, http: server.Client()}
	var v struct {
		ID string `json:"id"`
	}
	start := time.Now()
	if err := c.get(context.Background(), "/", &v); err != nil {
		t.Fatal(err)
	}
	if calls != 2 || v.ID != "ok" || time.Since(start) < time.Second {
		t.Fatal("retry failed")
	}
	ctx, cancel := context.WithCancel(context.Background())
	cancel()
	if err := c.get(ctx, "/", &v); err == nil {
		t.Fatal("cancellation ignored")
	}
}
func TestInvalidHTTPResponses(t *testing.T) {
	for _, status := range []int{200, 404} {
		t.Run(fmt.Sprint(status), func(t *testing.T) {
			calls := 0
			s := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) { calls++; w.WriteHeader(status); fmt.Fprint(w, "invalid") }))
			defer s.Close()
			c := &client{base: s.URL, http: s.Client()}
			var v struct {
				ID string `json:"id"`
			}
			if err := c.get(context.Background(), "/", &v); err == nil || calls != 1 {
				t.Fatal("invalid response accepted or retried")
			}
		})
	}
}

func TestFetchPreservesRawResponses(t *testing.T) {
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		if r.URL.Path == "/Line/Mode/tube,dlr" {
			fmt.Fprint(w, `[{"id":"test","name":"Test","unknown":{"keep":true}}]`)
			return
		}
		// Deliberately unusable station/route data must still be archived by fetch.
		fmt.Fprint(w, `{"lineId":"test","stations":[{"id":"outside","zone":"9"}],"orderedLineRoutes":[],"unknown":[1,2,3]}`)
	}))
	defer server.Close()
	data, err := fetchSnapshot(context.Background(), &client{base: server.URL, http: server.Client()})
	if err != nil {
		t.Fatal(err)
	}
	if len(data.Sequences) != 2 {
		t.Fatal("missing directional response")
	}
	var lines []map[string]json.RawMessage
	if err := json.Unmarshal(data.Lines, &lines); err != nil {
		t.Fatal(err)
	}
	if string(lines[0]["unknown"]) != `{"keep":true}` {
		t.Fatal("line fields lost")
	}
	for _, entry := range data.Sequences {
		var response map[string]json.RawMessage
		if err := json.Unmarshal(entry.Data, &response); err != nil {
			t.Fatal(err)
		}
		if string(response["unknown"]) != "[1,2,3]" || string(response["stations"]) != `[{"id":"outside","zone":"9"}]` {
			t.Fatal("raw fields lost")
		}
	}
	path := filepath.Join(t.TempDir(), "snapshot.json")
	if err := writeJSON(path, data); err != nil {
		t.Fatal(err)
	}
	raw, err := os.ReadFile(path)
	if err != nil || !json.Valid(raw) {
		t.Fatalf("invalid saved JSON: %v", err)
	}
}

func TestFetchTimetablesOncePerEndpointPair(t *testing.T) {
	requests := 0
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		switch r.URL.Path {
		case "/Line/Mode/tube,dlr":
			fmt.Fprint(w, `[{"id":"test"}]`)
		case "/Line/test/Timetable/A/to/C":
			requests++
			fmt.Fprint(w, `{"unknown":"preserved","timetable":{"routes":[]}}`)
		default:
			fmt.Fprint(w, `{"orderedLineRoutes":[{"naptanIds":["A","B","C"]},{"naptanIds":["A","D","C"]}]}`)
		}
	}))
	defer server.Close()
	data, err := fetchSnapshot(context.Background(), &client{base: server.URL, http: server.Client()})
	if err != nil {
		t.Fatal(err)
	}
	if requests != 1 || len(data.Timetables) != 1 {
		t.Fatalf("duplicate timetable requests: %d", requests)
	}
	if string(data.Timetables[0].Data) != `{"unknown":"preserved","timetable":{"routes":[]}}` {
		t.Fatal("raw timetable altered")
	}
}
