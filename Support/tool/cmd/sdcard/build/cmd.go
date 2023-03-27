package build

import (
	"github.com/melair/ktane/cmd/sdcard"
	"github.com/spf13/cobra"
)

// BuildCmd represents the build command
var BuildCmd = &cobra.Command{
	Use:   "build",
	Short: "Build a new SD card image",
	Long: `Builds a new SD card image with the assets provided in the location
provided (default "resources/").

Assets must be located in a directory with a numerical prefix that matches
a module number, further individual assets must be prefixed with their 
identity and have an extension of their type.

Asset Types:
    * .pcm - audio, no header, 8-bit unsigned, 16000Hz, mono

Example layouts: 
    resources/003-controller/000-time-beep-high.pcm
    resources/003-controller/001-time-beep-low.pcm
    resources/003-controller/002-strike.pcm
    resources/003-controller/003-gameover-success.pcm
`,
	RunE: func(cmd *cobra.Command, args []string) error {
		image := cmd.Flags().Lookup("image").Value.String()
		resources := cmd.Flags().Lookup("resources").Value.String()

		return buildImage(image, resources)
	},
}

func init() {
	sdcard.SDCardCmd.AddCommand(BuildCmd)

	BuildCmd.Flags().StringP("image", "i", "sdcard.img", "location of sdcard image to operate on")
	BuildCmd.Flags().StringP("resources", "r", "resources/", "location of assets to build into sdcard image")
}
