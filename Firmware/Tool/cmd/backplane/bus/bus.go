package bus

import (
	"github.com/melair/ktane/Firmware/Tool/cmd/backplane/bus/monitor"
	"github.com/spf13/cobra"
)

func NewCommand() *cobra.Command {
	busCmd := &cobra.Command{
		Use:   "bus",
		Short: "Work with the backplane bus",
	}

	busCmd.AddCommand(monitor.NewCommand())

	return busCmd
}
