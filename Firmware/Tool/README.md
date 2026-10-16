# KTANE Firmware Tool

Command-line support tooling for the KTANE firmware.

## Usage

The binary is intended to be used as a multi-command tool:

```sh
./tool font generate render --font font.ttf --text "HELLO" --height 16 --format png --out preview.png
./tool font generate render --font font.ttf --text "HELLO" --height 16 --format c --out font_render.c --prefix ktane_render
./tool font generate glyphs --font font.ttf --chars "ABCDEF12345" --height 16 --format c --out font.c --prefix ktane_font
./tool font generate glyphs --font font.ttf --chars "ABCDEF12345" --height 16 --format png --out font.png --prefix ktane_font
./tool sdcard fat format card.img --size 64
./tool sdcard fat add card.img beeps.pcm --mode 0x42 --meta 00000000
./tool sdcard fat list card.img
./tool sdcard fat extract card.img restored-beeps.pcm --mode 0x42 --type pcm --name beeps
./tool sdcard fat remove card.img --mode 0x42 --type pcm --name beeps
./tool sdcard fat defrag card.img
```
