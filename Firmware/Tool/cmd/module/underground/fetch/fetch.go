package fetch

import (
	"encoding/json"
	"fmt"
	"os"
	"os/signal"
	"path/filepath"
	"syscall"

	"github.com/spf13/cobra"
)

// NewCommand creates the command that archives unprocessed TfL API responses.
func NewCommand() *cobra.Command {
	var output string
	cmd := &cobra.Command{
		Use:   "fetch",
		Short: "Fetch data for the Underground module from the TfL Unified API",
		Args:  cobra.NoArgs,
		RunE: func(cmd *cobra.Command, args []string) error {
			if output == "" {
				return fmt.Errorf("--output is required")
			}
			ctx, stop := signal.NotifyContext(cmd.Context(), os.Interrupt, syscall.SIGTERM)
			defer stop()
			data, err := fetchSnapshot(ctx, newClient())
			if err != nil {
				return err
			}
			if err := writeJSON(output, data); err != nil {
				return err
			}
			_, err = fmt.Fprintf(cmd.OutOrStdout(), "saved %s: line discovery, %d route responses, %d timetables\n", output, len(data.Sequences), len(data.Timetables))
			return err
		},
	}
	cmd.Flags().StringVar(&output, "output", "", "Destination raw JSON snapshot path")
	_ = cmd.MarkFlagRequired("output")
	return cmd
}

func writeJSON(path string, data any) error {
	f, err := os.CreateTemp(filepath.Dir(path), ".underground-*.json")
	if err != nil {
		return err
	}
	defer os.Remove(f.Name())
	encoder := json.NewEncoder(f)
	encoder.SetIndent("", "  ")
	encoder.SetEscapeHTML(false)
	if err := encoder.Encode(data); err != nil {
		f.Close()
		return err
	}
	if err := f.Chmod(0644); err != nil {
		f.Close()
		return err
	}
	if err := f.Close(); err != nil {
		return err
	}
	return os.Rename(f.Name(), path)
}
