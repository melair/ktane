//
// Created by Peter Wood on 11/09/2026.
//

#ifndef STM32FIRMWARE_GLYPH_H
#define STM32FIRMWARE_GLYPH_H

#include <stdint.h>

typedef struct {
    char character;
    uint16_t width;
    uint16_t height;
    int16_t x_offset;
    int16_t y_offset;
    uint16_t x_advance;
    uint32_t data_offset;
} glyph_t;

#endif //STM32FIRMWARE_GLYPH_H
