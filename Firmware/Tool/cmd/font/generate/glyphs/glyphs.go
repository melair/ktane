package glyphs

import (
	"bytes"
	_ "embed"
	"errors"
	"fmt"
	"image"
	"image/color"
	"image/draw"
	"image/png"
	"math"
	"os"
	"strings"
	"text/template"

	"github.com/spf13/cobra"
	xdraw "golang.org/x/image/draw"
	"golang.org/x/image/font"
	"golang.org/x/image/font/opentype"
	"golang.org/x/image/math/fixed"
)

//go:embed c_template.c.tmpl
var cTemplateSource string

var cTemplate = template.Must(template.New("glyphs_c").Parse(cTemplateSource))

type options struct {
	fontPath string
	chars    string
	height   int
	format   string
	outPath  string
	prefix   string
}

type glyph struct {
	char       byte
	width      int
	height     int
	xOffset    int
	yOffset    int
	xAdvance   int
	dataOffset int
	bitmap     *image.Gray
}

type fontData struct {
	prefix     string
	lineHeight int
	ascent     int
	descent    int
	glyphs     []glyph
	asciiMap   [128]int
	bitmap     []byte
}

type cTemplateData struct {
	Prefix     string
	LineHeight int
	Ascent     int
	Descent    int
	GlyphCount int
	AsciiMap   string
	Glyphs     string
	Bitmap     string
}

func NewCommand() *cobra.Command {
	opts := options{format: "c", prefix: "font"}

	cmd := &cobra.Command{
		Use:   "glyphs",
		Short: "Generate firmware bitmap font glyph data",
		RunE: func(cmd *cobra.Command, args []string) error {
			return run(opts)
		},
	}

	cmd.Flags().StringVar(&opts.fontPath, "font", "", "Path to a TrueType font file")
	cmd.Flags().StringVar(&opts.chars, "chars", "", "ASCII characters to include")
	cmd.Flags().IntVar(&opts.height, "height", 0, "Target common glyph bounds height in pixels")
	cmd.Flags().StringVar(&opts.format, "format", opts.format, "Output format: c or png")
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

	chars := deduplicateASCII(opts.chars)
	data, err := generateFontData(ttf, chars, opts.height, opts.prefix)
	if err != nil {
		return err
	}

	switch opts.format {
	case "c":
		return writeC(opts.outPath, data)
	case "png":
		return writeAtlasPNG(opts.outPath, data)
	default:
		return fmt.Errorf("unsupported format %q: expected c or png", opts.format)
	}
}

func validate(opts options) error {
	switch {
	case opts.fontPath == "":
		return errors.New("--font is required")
	case opts.chars == "":
		return errors.New("--chars is required")
	case opts.height <= 0:
		return errors.New("--height must be greater than zero")
	case opts.outPath == "":
		return errors.New("--out is required")
	case opts.prefix == "":
		return errors.New("--prefix is required")
	}

	if !isCIdentifier(opts.prefix) {
		return fmt.Errorf("--prefix %q is not a valid C identifier", opts.prefix)
	}

	for i := 0; i < len(opts.chars); i++ {
		if opts.chars[i] > 127 {
			return fmt.Errorf("--chars contains non-ASCII character at byte %d", i)
		}
		if opts.chars[i] == '\n' || opts.chars[i] == '\r' {
			return errors.New("--chars must be a single line")
		}
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

func deduplicateASCII(chars string) []byte {
	seen := [128]bool{}
	deduplicated := make([]byte, 0, len(chars))

	for i := 0; i < len(chars); i++ {
		ch := chars[i]
		if seen[ch] {
			continue
		}

		seen[ch] = true
		deduplicated = append(deduplicated, ch)
	}

	return deduplicated
}

func generateFontData(ttf *opentype.Font, chars []byte, targetHeight int, prefix string) (fontData, error) {
	sourceHeight := max(64, targetHeight*8)
	face, err := opentype.NewFace(ttf, &opentype.FaceOptions{
		Size:    float64(sourceHeight),
		DPI:     72,
		Hinting: font.HintingNone,
	})
	if err != nil {
		return fontData{}, fmt.Errorf("create font face: %w", err)
	}
	defer face.Close()

	sourceGlyphs := make([]glyph, 0, len(chars))
	commonMinY := math.MaxInt
	commonMaxY := math.MinInt

	for _, ch := range chars {
		rendered, err := renderGlyph(face, ch)
		if err != nil {
			return fontData{}, err
		}

		if rendered.height > 0 {
			commonMinY = min(commonMinY, rendered.yOffset)
			commonMaxY = max(commonMaxY, rendered.yOffset+rendered.height)
		}

		sourceGlyphs = append(sourceGlyphs, rendered)
	}

	if commonMaxY <= commonMinY {
		return fontData{}, errors.New("selected characters rendered no visible glyph pixels")
	}

	scale := float64(targetHeight) / float64(commonMaxY-commonMinY)
	data := fontData{
		prefix:     prefix,
		lineHeight: targetHeight,
		ascent:     int(math.Round(float64(-commonMinY) * scale)),
		descent:    targetHeight - int(math.Round(float64(-commonMinY)*scale)),
	}

	for i := range data.asciiMap {
		data.asciiMap[i] = -1
	}

	for index, sourceGlyph := range sourceGlyphs {
		scaled := scaleGlyph(sourceGlyph, scale)
		scaled.dataOffset = len(data.bitmap)
		data.bitmap = append(data.bitmap, packBitmap(scaled.bitmap)...)
		data.asciiMap[scaled.char] = index
		data.glyphs = append(data.glyphs, scaled)
	}

	return data, nil
}

func renderGlyph(face font.Face, ch byte) (glyph, error) {
	r := rune(ch)
	advance, ok := face.GlyphAdvance(r)
	if !ok {
		return glyph{}, fmt.Errorf("font does not contain glyph for ASCII %d", ch)
	}

	result := glyph{
		char:     ch,
		xAdvance: ceilFixed(advance),
	}

	bounds, _ := font.BoundString(face, string(r))
	if bounds.Empty() {
		return result, nil
	}

	sourceBounds := fixedRectToImageRect(bounds)
	padding := 2
	canvas := image.NewGray(sourceBounds.Sub(sourceBounds.Min).Add(image.Pt(padding*2, padding*2)))
	origin := image.Pt(padding-sourceBounds.Min.X, padding-sourceBounds.Min.Y)

	drawer := font.Drawer{
		Dst:  canvas,
		Src:  image.White,
		Face: face,
		Dot: fixed.Point26_6{
			X: fixed.I(origin.X),
			Y: fixed.I(origin.Y),
		},
	}
	drawer.DrawString(string(r))

	trimmed, offset, ok := trim(canvas)
	if !ok {
		return result, nil
	}

	result.width = trimmed.Bounds().Dx()
	result.height = trimmed.Bounds().Dy()
	result.xOffset = offset.X - origin.X
	result.yOffset = offset.Y - origin.Y
	result.bitmap = trimmed

	return result, nil
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

func trim(src *image.Gray) (*image.Gray, image.Point, bool) {
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
		return nil, image.Point{}, false
	}

	trimmedBounds := image.Rect(0, 0, maxX-minX, maxY-minY)
	trimmed := image.NewGray(trimmedBounds)
	draw.Draw(trimmed, trimmedBounds, src, image.Pt(minX, minY), draw.Src)

	return trimmed, image.Pt(minX, minY), true
}

func scaleGlyph(source glyph, scale float64) glyph {
	scaled := glyph{
		char:     source.char,
		xOffset:  int(math.Round(float64(source.xOffset) * scale)),
		yOffset:  int(math.Round(float64(source.yOffset) * scale)),
		xAdvance: max(1, int(math.Round(float64(source.xAdvance)*scale))),
	}

	if source.bitmap == nil || source.width == 0 || source.height == 0 {
		return scaled
	}

	width := max(1, int(math.Round(float64(source.width)*scale)))
	height := max(1, int(math.Round(float64(source.height)*scale)))
	scaledBounds := image.Rect(0, 0, width, height)
	gray := image.NewGray(scaledBounds)
	xdraw.ApproxBiLinear.Scale(gray, scaledBounds, source.bitmap, source.bitmap.Bounds(), draw.Over, nil)

	bitmap := image.NewGray(scaledBounds)
	for y := scaledBounds.Min.Y; y < scaledBounds.Max.Y; y++ {
		for x := scaledBounds.Min.X; x < scaledBounds.Max.X; x++ {
			if gray.GrayAt(x, y).Y >= 128 {
				bitmap.SetGray(x, y, color.Gray{Y: 255})
			}
		}
	}

	scaled.width = width
	scaled.height = height
	scaled.bitmap = bitmap

	return scaled
}

func packBitmap(bitmap *image.Gray) []byte {
	if bitmap == nil {
		return nil
	}

	bounds := bitmap.Bounds()
	bytesPerRow := (bounds.Dx() + 7) / 8
	packed := make([]byte, 0, bytesPerRow*bounds.Dy())

	for y := bounds.Min.Y; y < bounds.Max.Y; y++ {
		for byteIndex := 0; byteIndex < bytesPerRow; byteIndex++ {
			var value byte
			for bit := 0; bit < 8; bit++ {
				x := bounds.Min.X + byteIndex*8 + bit
				if x < bounds.Max.X && bitmap.GrayAt(x, y).Y != 0 {
					value |= 1 << (7 - bit)
				}
			}
			packed = append(packed, value)
		}
	}

	return packed
}

func writeC(path string, data fontData) error {
	var output bytes.Buffer
	templateData := cTemplateData{
		Prefix:     data.prefix,
		LineHeight: data.lineHeight,
		Ascent:     data.ascent,
		Descent:    data.descent,
		GlyphCount: len(data.glyphs),
		AsciiMap:   formatAsciiMap(data.asciiMap),
		Glyphs:     formatGlyphs(data.glyphs),
		Bitmap:     formatBitmap(data.bitmap),
	}

	if err := cTemplate.Execute(&output, templateData); err != nil {
		return fmt.Errorf("render c template: %w", err)
	}

	if err := os.WriteFile(path, output.Bytes(), 0644); err != nil {
		return fmt.Errorf("write c output: %w", err)
	}

	return nil
}

func formatAsciiMap(asciiMap [128]int) string {
	var builder strings.Builder

	for i := 0; i < 128; i += 8 {
		builder.WriteString("\t")
		for j := 0; j < 8; j++ {
			fmt.Fprintf(&builder, "%d,", asciiMap[i+j])
			if j != 7 {
				builder.WriteString(" ")
			}
		}
		builder.WriteString("\n")
	}

	return builder.String()
}

func formatGlyphs(glyphs []glyph) string {
	var builder strings.Builder

	for _, glyph := range glyphs {
		fmt.Fprintf(
			&builder,
			"\t{%s, %d, %d, %d, %d, %d, %d},\n",
			cChar(glyph.char),
			glyph.width,
			glyph.height,
			glyph.xOffset,
			glyph.yOffset,
			glyph.xAdvance,
			glyph.dataOffset,
		)
	}

	return builder.String()
}

func formatBitmap(bitmap []byte) string {
	var builder strings.Builder

	for i := 0; i < len(bitmap); i += 12 {
		builder.WriteString("\t")
		for j := 0; j < 12 && i+j < len(bitmap); j++ {
			fmt.Fprintf(&builder, "0x%02x,", bitmap[i+j])
			if j != 11 && i+j+1 < len(bitmap) {
				builder.WriteString(" ")
			}
		}
		builder.WriteString("\n")
	}

	return builder.String()
}

func cChar(ch byte) string {
	switch ch {
	case '\'':
		return "'\\''"
	case '\\':
		return "'\\\\'"
	case '\t':
		return "'\\t'"
	case '\v':
		return "'\\v'"
	case '\f':
		return "'\\f'"
	case '\b':
		return "'\\b'"
	case '\a':
		return "'\\a'"
	case 0:
		return "'\\0'"
	default:
		if ch >= 32 && ch <= 126 {
			return fmt.Sprintf("'%c'", ch)
		}
		return fmt.Sprintf("%d", ch)
	}
}

func writeAtlasPNG(path string, data fontData) error {
	const padding = 4

	maxCellWidth := 1
	for _, glyph := range data.glyphs {
		left := min(0, glyph.xOffset)
		right := max(glyph.xAdvance, glyph.xOffset+glyph.width)
		maxCellWidth = max(maxCellWidth, right-left)
	}

	cellWidth := maxCellWidth + padding*2
	cellHeight := data.lineHeight + padding*2
	columns := int(math.Ceil(math.Sqrt(float64(len(data.glyphs)))))
	rows := int(math.Ceil(float64(len(data.glyphs)) / float64(columns)))
	atlas := image.NewGray(image.Rect(0, 0, columns*cellWidth+1, rows*cellHeight+1))
	draw.Draw(atlas, atlas.Bounds(), image.White, image.Point{}, draw.Src)

	grid := color.Gray{Y: 220}
	baseline := color.Gray{Y: 180}
	black := color.Gray{Y: 0}

	for row := 0; row <= rows; row++ {
		y := row * cellHeight
		for x := 0; x < atlas.Bounds().Dx(); x++ {
			atlas.SetGray(x, y, grid)
		}
	}
	for column := 0; column <= columns; column++ {
		x := column * cellWidth
		for y := 0; y < atlas.Bounds().Dy(); y++ {
			atlas.SetGray(x, y, grid)
		}
	}

	for index, glyph := range data.glyphs {
		column := index % columns
		row := index / columns
		cellX := column * cellWidth
		cellY := row * cellHeight
		baselineY := cellY + padding + data.ascent

		for x := cellX + 1; x < cellX+cellWidth; x++ {
			atlas.SetGray(x, baselineY, baseline)
		}

		if glyph.bitmap == nil {
			continue
		}

		dstX := cellX + padding + glyph.xOffset
		dstY := baselineY + glyph.yOffset
		for y := 0; y < glyph.height; y++ {
			for x := 0; x < glyph.width; x++ {
				if glyph.bitmap.GrayAt(x, y).Y != 0 {
					atlas.SetGray(dstX+x, dstY+y, black)
				}
			}
		}
	}

	file, err := os.Create(path)
	if err != nil {
		return fmt.Errorf("create png output: %w", err)
	}
	defer file.Close()

	if err := png.Encode(file, atlas); err != nil {
		return fmt.Errorf("write png output: %w", err)
	}

	return nil
}
