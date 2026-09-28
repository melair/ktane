package process

import (
	"encoding/json"
	"fmt"
	"os"

	"github.com/melair/ktane/Firmware/Tool/cmd/module/underground/internal/snapshot"
	"github.com/spf13/cobra"
)

// NewCommand processes a saved TfL snapshot without making network requests.
func NewCommand() *cobra.Command {
	var input, output string
	var debug bool
	cmd := &cobra.Command{
		Use: "process", Short: "Process a TfL JSON snapshot into an Underground C header", Args: cobra.NoArgs,
		RunE: func(cmd *cobra.Command, args []string) error {
			if input == "" {
				return fmt.Errorf("--input is required")
			}
			if !debug && output == "" {
				return fmt.Errorf("either --output or --debug is required")
			}
			raw, err := os.ReadFile(input)
			if err != nil {
				return err
			}
			data, err := processSnapshot(raw)
			if err != nil {
				return err
			}
			if debug {
				return writeDebug(cmd.OutOrStdout(), data)
			}
			if err := writeHeader(output, data); err != nil {
				return err
			}
			_, err = fmt.Fprintf(cmd.OutOrStdout(), "generated %s: %d lines, %d stations, %d routes\n", output, len(data.Lines), len(data.Stations), len(data.Routes))
			return err
		},
	}
	cmd.Flags().StringVar(&input, "input", "", "Source raw JSON snapshot path")
	cmd.Flags().StringVar(&output, "output", "", "Destination C header path")
	cmd.Flags().BoolVar(&debug, "debug", false, "Print stations and their routes, then ordered route stops, to stdout")
	_ = cmd.MarkFlagRequired("input")
	cmd.MarkFlagsMutuallyExclusive("output", "debug")
	cmd.MarkFlagsOneRequired("output", "debug")
	return cmd
}

func processSnapshot(raw []byte) (*dataset, error) {
	var archive snapshot.Data
	if err := json.Unmarshal(raw, &archive); err != nil {
		return nil, fmt.Errorf("decode snapshot: %w", err)
	}
	if archive.Version != 1 {
		return nil, fmt.Errorf("unsupported snapshot version %d", archive.Version)
	}
	var lines []line
	if err := json.Unmarshal(archive.Lines, &lines); err != nil {
		return nil, fmt.Errorf("decode lines: %w", err)
	}
	if len(lines) == 0 {
		return nil, fmt.Errorf("snapshot has no lines")
	}
	expected := map[string]bool{}
	for _, l := range lines {
		if l.ID == "" || l.Name == "" {
			return nil, fmt.Errorf("invalid line metadata")
		}
		for _, direction := range []string{"inbound", "outbound"} {
			key := l.ID + "/" + direction
			if _, ok := expected[key]; ok {
				return nil, fmt.Errorf("duplicate line %s", l.ID)
			}
			expected[key] = false
		}
	}
	var sequences []sequence
	for _, entry := range archive.Sequences {
		key := entry.LineID + "/" + entry.Direction
		seen, ok := expected[key]
		if !ok || seen {
			return nil, fmt.Errorf("unexpected or duplicate route response %s", key)
		}
		var seq sequence
		if err := json.Unmarshal(entry.Data, &seq); err != nil {
			return nil, fmt.Errorf("decode %s: %w", key, err)
		}
		if seq.LineID != entry.LineID || len(seq.Routes) == 0 {
			return nil, fmt.Errorf("incomplete route response %s", key)
		}
		expected[key] = true
		sequences = append(sequences, seq)
	}
	for _, l := range lines {
		for _, direction := range []string{"inbound", "outbound"} {
			if !expected[l.ID+"/"+direction] {
				return nil, fmt.Errorf("missing route response %s/%s", l.ID, direction)
			}
		}
	}
	sequences, omissions, err := reconcileTimetables(sequences, archive.Timetables)
	if err != nil {
		return nil, err
	}
	data, err := buildDataset(lines, sequences)
	if err != nil {
		return nil, err
	}
	if err := addFrequencies(data, archive.Timetables); err != nil {
		return nil, err
	}
	for _, r := range data.Routes {
		if r.Frequency.Source == frequencyUnknown {
			return nil, fmt.Errorf("cannot establish daytime frequency for %s: %s -> %s: %s", data.Lines[r.Line].Name, data.Stations[r.Origin].Name, data.Stations[r.Destination].Name, r.Frequency.Note)
		}
	}
	if err := filterDaytimeRoutes(data); err != nil {
		return nil, err
	}
	data.TimetableOmissions = omissions
	return data, nil
}
