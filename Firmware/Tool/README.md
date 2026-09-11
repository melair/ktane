# KTANE Firmware Tool

Command-line support tooling for the KTANE firmware.

## Usage

```sh
go run . font generate render --font font.ttf --text "HELLO" --height 16 --format png --out preview.png
go run . font generate render --font font.ttf --text "HELLO" --height 16 --format c --out font_render.c --prefix ktane_render
go run . font generate glyphs --font font.ttf --chars "ABCDEF12345" --height 16 --format c --out font.c --prefix ktane_font
go run . font generate glyphs --font font.ttf --chars "ABCDEF12345" --height 16 --format png --out font.png --prefix ktane_font
```

The binary is intended to be used as a multi-command tool:

```sh
./tool font generate render --font font.ttf --text "HELLO" --height 16 --format png --out preview.png
./tool font generate render --font font.ttf --text "HELLO" --height 16 --format c --out font_render.c --prefix ktane_render
./tool font generate glyphs --font font.ttf --chars "ABCDEF12345" --height 16 --format c --out font.c --prefix ktane_font
./tool font generate glyphs --font font.ttf --chars "ABCDEF12345" --height 16 --format png --out font.png --prefix ktane_font
```
