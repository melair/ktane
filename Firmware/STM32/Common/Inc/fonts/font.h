#ifndef STM32FIRMWARE_FONT_H
#define STM32FIRMWARE_FONT_H

#include <stdint.h>

#include "glyph.h"

typedef struct {
    const glyph_t *glyphs;
    uint16_t glyph_count;
    const int16_t *character_map;
    uint16_t character_map_count;
    const uint8_t *bitmap;
    uint16_t line_height;
    int16_t ascent;
} font_t;

#endif // STM32FIRMWARE_FONT_H
