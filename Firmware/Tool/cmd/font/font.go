package font

import (
	"github.com/melair/ktane/Firmware/Tool/cmd/font/generate"
	"github.com/spf13/cobra"
)

func NewCommand() *cobra.Command {
	fontCmd := &cobra.Command{
		Use:   "font",
		Short: "Work with firmware font assets",
	}

	fontCmd.AddCommand(generate.NewCommand())

	return fontCmd
}
