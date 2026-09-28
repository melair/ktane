package underground

import (
	"github.com/melair/ktane/Firmware/Tool/cmd/module/underground/fetch"
	"github.com/melair/ktane/Firmware/Tool/cmd/module/underground/process"
	"github.com/spf13/cobra"
)

// NewCommand creates the command group for the Underground module tooling.
func NewCommand() *cobra.Command {
	undergroundCmd := &cobra.Command{
		Use:   "underground",
		Short: "Work with the Underground module",
	}

	undergroundCmd.AddCommand(fetch.NewCommand())
	undergroundCmd.AddCommand(process.NewCommand())

	return undergroundCmd
}
