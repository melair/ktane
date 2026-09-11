package generate

import (
	"github.com/melair/ktane/Firmware/Tool/cmd/font/generate/glyphs"
	"github.com/melair/ktane/Firmware/Tool/cmd/font/generate/render"
	"github.com/spf13/cobra"
)

func NewCommand() *cobra.Command {
	generateCmd := &cobra.Command{
		Use:   "generate",
		Short: "Generate firmware font assets",
	}

	generateCmd.AddCommand(render.NewCommand())
	generateCmd.AddCommand(glyphs.NewCommand())

	return generateCmd
}
