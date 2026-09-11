package cmd

import (
	"github.com/melair/ktane/Firmware/Tool/cmd/font"
	"github.com/spf13/cobra"
)

func Execute() error {
	return newRootCommand().Execute()
}

func newRootCommand() *cobra.Command {
	rootCmd := &cobra.Command{
		Use:           "tool",
		Short:         "Support tooling for KTANE firmware development",
		SilenceUsage:  true,
		SilenceErrors: true,
	}

	rootCmd.AddCommand(font.NewCommand())

	return rootCmd
}
