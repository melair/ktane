package pianokeys

import (
	"github.com/melair/ktane/Firmware/Tool/cmd/module/pianokeys/midi"
	"github.com/spf13/cobra"
)

func NewCommand() *cobra.Command {
	pianokeysCmd := &cobra.Command{
		Use:   "pianokeys",
		Short: "Work with the Piano Keys module",
	}

	pianokeysCmd.AddCommand(midi.NewCommand())

	return pianokeysCmd
}
