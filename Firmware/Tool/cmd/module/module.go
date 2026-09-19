package module

import (
	"github.com/melair/ktane/Firmware/Tool/cmd/module/pianokeys"
	"github.com/spf13/cobra"
)

func NewCommand() *cobra.Command {
	moduleCmd := &cobra.Command{
		Use:   "module",
		Short: "Work with KTANE modules",
	}

	moduleCmd.AddCommand(pianokeys.NewCommand())

	return moduleCmd
}
