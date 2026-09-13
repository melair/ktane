package edgework

import (
	"github.com/melair/ktane/Firmware/Tool/cmd/edgework/bus"
	"github.com/spf13/cobra"
)

func NewCommand() *cobra.Command {
	edgeworkCmd := &cobra.Command{
		Use:   "edgework",
		Short: "Work with edgework modules",
	}

	edgeworkCmd.AddCommand(bus.NewCommand())

	return edgeworkCmd
}
