package sdcard

import (
	"github.com/melair/ktane/Firmware/Tool/cmd/sdcard/fat"
	"github.com/spf13/cobra"
)

// NewCommand creates the SD card tooling command group.
func NewCommand() *cobra.Command {
	cmd := &cobra.Command{
		Use:   "sdcard",
		Short: "Work with KTANE SD cards and images",
	}
	cmd.AddCommand(fat.NewCommand())
	return cmd
}
