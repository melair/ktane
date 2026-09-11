package render

import (
	"errors"
	"fmt"
	"image"
	"image/color"
	"image/draw"
	"image/png"
	"math"
	"os"
	"strings"

	"github.com/spf13/cobra"
	xdraw "golang.org/x/image/draw"
	"golang.org/x/image/font"
	"golang.org/x/image/font/opentype"
	"golang.org/x/image/math/fixed"
)

type options struct {
	fontPath string
	text     string
	height   int
	format   string
	outPath  string
	prefix   string
}

func NewCommand() *cobra.Command {
	opts := options{prefix: "font_render"}

	cmd := &cobra.Command{
		Use:   "render",
		Short: "Render a firmware font preview",
		RunE: func(cmd *cobra.Command, args []string) error {
			return run(opts)
		},
	}

	cmd.Flags().StringVar(&opts.fontPath, "font", "", "Path to a TrueType font file")
	cmd.Flags().StringVar(&opts.text, "text", "", "Single line text to render")
	cmd.Flags().IntVar(&opts.height, "height", 0, "Target rendered text height in pixels")
	cmd.Flags().StringVar(&opts.format, "format", "png", "Output format: png or c")
	cmd.Flags().StringVar(&opts.outPath, "out", "", "Output file path")
	cmd.Flags().StringVar(&opts.prefix, "prefix", opts.prefix, "C symbol prefix")

	return cmd
}

func run(opts options) error {
	if err := validate(opts); err != nil {
		return err
	}

	fontBytes, err := os.ReadFile(opts.fontPath)
	if err != nil {
		return fmt.Errorf("read font: %w", err)
	}

	ttf, err := opentype.Parse(fontBytes)
	if err != nil {
		return fmt.Errorf("parse TrueType font: %w", err)
	}

	bitmap, err := renderText(ttf, opts.text, opts.height)
	if err != nil {
		return err
	}

	switch opts.format {
	case "png":
		return writePNG(opts.outPath, bitmap)
	case "c":
		return writeCArray(opts.outPath, bitmap, opts.prefix)
	default:
		return fmt.Errorf("unsupported format %q: expected png or c", opts.format)
	}
}

func validate(opts options) error {
	switch {
	case opts.fontPath == "":
		return errors.New("--font is required")
	case opts.text == "":
		return errors.New("--text is required")
	case strings.ContainsAny(opts.text, "\r\n"):
		return errors.New("--text must be a single line")
	case opts.height <= 0:
		return errors.New("--height must be greater than zero")
	case opts.outPath == "":
		return errors.New("--out is required")
	case opts.prefix == "":
		return errors.New("--prefix is required")
	case !isCIdentifier(opts.prefix):
		return fmt.Errorf("--prefix %q is not a valid C identifier", opts.prefix)
	}

	return nil
}

func isCIdentifier(value string) bool {
	for i := 0; i < len(value); i++ {
		ch := value[i]
		valid := ch == '_' || ('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z') || (i > 0 && '0' <= ch && ch <= '9')
		if !valid {
			return false
		}
	}

	return value[0] == '_' || ('a' <= value[0] && value[0] <= 'z') || ('A' <= value[0] && value[0] <= 'Z')
}

func renderText(ttf *opentype.Font, text string, targetHeight int) (*image.Gray, error) {
	sourceHeight := max(64, targetHeight*8)
	face, err := opentype.NewFace(ttf, &opentype.FaceOptions{
		Size:    float64(sourceHeight),
		DPI:     72,
		Hinting: font.HintingNone,
	})
	if err != nil {
		return nil, fmt.Errorf("create font face: %w", err)
	}
	defer face.Close()

	bounds, _ := font.BoundString(face, text)
	if bounds.Empty() {
		return nil, errors.New("text has an empty rendered bounds")
	}

	sourceBounds := fixedRectToImageRect(bounds)
	canvas := image.NewGray(sourceBounds.Sub(sourceBounds.Min).Add(image.Pt(4, 4)))

	drawer := font.Drawer{
		Dst:  canvas,
		Src:  image.White,
		Face: face,
		Dot: fixed.Point26_6{
			X: fixed.I(2 - sourceBounds.Min.X),
			Y: fixed.I(2 - sourceBounds.Min.Y),
		},
	}
	drawer.DrawString(text)

	trimmed, err := trimAlpha(canvas)
	if err != nil {
		return nil, err
	}

	return scaleAndThreshold(trimmed, targetHeight), nil
}

func fixedRectToImageRect(rect fixed.Rectangle26_6) image.Rectangle {
	return image.Rect(
		floorFixed(rect.Min.X),
		floorFixed(rect.Min.Y),
		ceilFixed(rect.Max.X),
		ceilFixed(rect.Max.Y),
	)
}

func floorFixed(value fixed.Int26_6) int {
	return int(math.Floor(float64(value) / 64.0))
}

func ceilFixed(value fixed.Int26_6) int {
	return int(math.Ceil(float64(value) / 64.0))
}

func trimAlpha(src *image.Gray) (*image.Gray, error) {
	bounds := src.Bounds()
	minX, minY := bounds.Max.X, bounds.Max.Y
	maxX, maxY := bounds.Min.X, bounds.Min.Y

	for y := bounds.Min.Y; y < bounds.Max.Y; y++ {
		for x := bounds.Min.X; x < bounds.Max.X; x++ {
			if src.GrayAt(x, y).Y == 0 {
				continue
			}

			minX = min(minX, x)
			minY = min(minY, y)
			maxX = max(maxX, x+1)
			maxY = max(maxY, y+1)
		}
	}

	if maxX <= minX || maxY <= minY {
		return nil, errors.New("text rendered no visible pixels")
	}

	trimmedBounds := image.Rect(0, 0, maxX-minX, maxY-minY)
	trimmed := image.NewGray(trimmedBounds)
	draw.Draw(trimmed, trimmedBounds, src, image.Pt(minX, minY), draw.Src)

	return trimmed, nil
}

func scaleAndThreshold(src *image.Gray, targetHeight int) *image.Gray {
	srcBounds := src.Bounds()
	targetWidth := max(1, int(math.Round(float64(srcBounds.Dx())*float64(targetHeight)/float64(srcBounds.Dy()))))
	scaledBounds := image.Rect(0, 0, targetWidth, targetHeight)
	scaled := image.NewGray(scaledBounds)
	xdraw.ApproxBiLinear.Scale(scaled, scaledBounds, src, srcBounds, draw.Over, nil)

	bitmap := image.NewGray(scaledBounds)
	for y := scaledBounds.Min.Y; y < scaledBounds.Max.Y; y++ {
		for x := scaledBounds.Min.X; x < scaledBounds.Max.X; x++ {
			if scaled.GrayAt(x, y).Y >= 128 {
				bitmap.SetGray(x, y, color.Gray{Y: 255})
			}
		}
	}

	return bitmap
}

func writePNG(path string, bitmap *image.Gray) error {
	out := image.NewGray(bitmap.Bounds())
	draw.Draw(out, out.Bounds(), image.White, image.Point{}, draw.Src)

	for y := bitmap.Bounds().Min.Y; y < bitmap.Bounds().Max.Y; y++ {
		for x := bitmap.Bounds().Min.X; x < bitmap.Bounds().Max.X; x++ {
			if bitmap.GrayAt(x, y).Y != 0 {
				out.SetGray(x, y, color.Gray{Y: 0})
			}
		}
	}

	file, err := os.Create(path)
	if err != nil {
		return fmt.Errorf("create png output: %w", err)
	}
	defer file.Close()

	if err := png.Encode(file, out); err != nil {
		return fmt.Errorf("write png output: %w", err)
	}

	return nil
}

func writeCArray(path string, bitmap *image.Gray, prefix string) error {
	file, err := os.Create(path)
	if err != nil {
		return fmt.Errorf("create c output: %w", err)
	}
	defer file.Close()

	bounds := bitmap.Bounds()
	width := bounds.Dx()
	height := bounds.Dy()
	bytesPerRow := (width + 7) / 8

	if _, err := fmt.Fprintf(file, "#include <stdint.h>\n\n"); err != nil {
		return err
	}
	if _, err := fmt.Fprintf(file, "const uint16_t %s_width = %d;\n", prefix, width); err != nil {
		return err
	}
	if _, err := fmt.Fprintf(file, "const uint16_t %s_height = %d;\n", prefix, height); err != nil {
		return err
	}
	if _, err := fmt.Fprintf(file, "const uint16_t %s_bytes_per_row = %d;\n\n", prefix, bytesPerRow); err != nil {
		return err
	}
	if _, err := fmt.Fprintf(file, "const uint8_t %s_bitmap[] = {\n", prefix); err != nil {
		return err
	}

	for y := bounds.Min.Y; y < bounds.Max.Y; y++ {
		if _, err := fmt.Fprint(file, "\t"); err != nil {
			return err
		}

		for byteIndex := 0; byteIndex < bytesPerRow; byteIndex++ {
			var value byte
			for bit := 0; bit < 8; bit++ {
				x := bounds.Min.X + byteIndex*8 + bit
				if x < bounds.Max.X && bitmap.GrayAt(x, y).Y != 0 {
					value |= 1 << (7 - bit)
				}
			}

			if _, err := fmt.Fprintf(file, "0x%02x,", value); err != nil {
				return err
			}
			if byteIndex != bytesPerRow-1 {
				if _, err := fmt.Fprint(file, " "); err != nil {
					return err
				}
			}
		}

		if _, err := fmt.Fprintln(file); err != nil {
			return err
		}
	}

	if _, err := fmt.Fprintln(file, "};"); err != nil {
		return err
	}

	return nil
}
