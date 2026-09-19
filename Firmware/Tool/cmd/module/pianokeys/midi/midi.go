package midi

import (
	"github.com/melair/ktane/Firmware/Tool/cmd/module/pianokeys/midi/generate"
	"github.com/spf13/cobra"
)

func NewCommand() *cobra.Command {
	midiCmd := &cobra.Command{
		Use:   "midi",
		Short: "Work with Piano Keys MIDI assets",
	}

	midiCmd.AddCommand(generate.NewCommand())

	return midiCmd
}
