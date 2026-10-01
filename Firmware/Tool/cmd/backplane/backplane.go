package backplane

import (
	"github.com/melair/ktane/Firmware/Tool/cmd/backplane/bus"
	"github.com/spf13/cobra"
)

func NewCommand() *cobra.Command {
	backplaneCmd := &cobra.Command{
		Use:   "backplane",
		Short: "Work with backplanes",
	}

	backplaneCmd.AddCommand(bus.NewCommand())

	return backplaneCmd
}
