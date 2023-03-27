package sdcard

import (
	"github.com/melair/ktane/cmd"
	"github.com/spf13/cobra"
)

// SDCardCmd represents the sdcard command
var SDCardCmd = &cobra.Command{
	Use:   "sdcard",
	Short: "Tooling to manage an SD card image",
	Long: `Tooling that is used to build, inspect or extract assets
from a KTANE sd card image.`,
}

func init() {
	cmd.RootCmd.AddCommand(SDCardCmd)
}
