#include "fonts/render.h"

#include <stdbool.h>
#include <stddef.h>

static const glyph_t *font_glyph(const font_t *font, const uint8_t character) {
    if ((font == NULL) || (font->glyphs == NULL) || (font->character_map == NULL) ||
        (character >= font->character_map_count)) {
        return NULL;
    }

    const int16_t index = font->character_map[character];
    if ((index < 0) || ((uint16_t) index >= font->glyph_count)) {
        return NULL;
    }

    return &font->glyphs[index];
}

void Font_Text(Font_SetPixel set_pixel, void *context, const uint8_t *text, const uint16_t length,
                 const font_t *font, const Font_Bounds bounds,
                 const Font_HorizontalAlign horizontal_align,
                 const Font_VerticalAlign vertical_align, const uint8_t colour) {
    if ((set_pixel == NULL) || (text == NULL) || (font == NULL) || (font->bitmap == NULL) ||
        ((unsigned) horizontal_align > FONT_ALIGN_RIGHT) ||
        ((unsigned) vertical_align > FONT_ALIGN_MIDDLE_BODY)) {
        return;
    }

    const uint16_t minimum_height = vertical_align == FONT_ALIGN_MIDDLE_BODY ?
                                        (uint16_t) font->ascent : font->line_height;
    if ((bounds.width == 0U) || (bounds.height < minimum_height)) {
        return;
    }

    bool has_glyph = false;
    int32_t text_left = 0;
    int32_t text_right = 0;
    int32_t cursor_x = 0;
    for (uint16_t index = 0U; index < length; index++) {
        const glyph_t *glyph = font_glyph(font, text[index]);
        if (glyph == NULL) {
            continue;
        }

        const int32_t glyph_left = cursor_x + glyph->x_offset;
        const int32_t glyph_right = glyph_left + glyph->width;
        if (!has_glyph || (glyph_left < text_left)) {
            text_left = glyph_left;
        }
        if (!has_glyph || (glyph_right > text_right)) {
            text_right = glyph_right;
        }
        has_glyph = true;
        cursor_x += glyph->x_advance;
    }

    if (!has_glyph) {
        return;
    }

    const int32_t text_width = text_right - text_left;
    int32_t cursor_start = bounds.x - text_left;
    if (horizontal_align == FONT_ALIGN_CENTRE) {
        cursor_start += ((int32_t) bounds.width - text_width) / 2;
    } else if (horizontal_align == FONT_ALIGN_RIGHT) {
        cursor_start += (int32_t) bounds.width - text_width;
    }

    int32_t baseline = bounds.y + font->ascent;
    if (vertical_align == FONT_ALIGN_MIDDLE) {
        baseline += ((int32_t) bounds.height - font->line_height) / 2;
    } else if (vertical_align == FONT_ALIGN_BOTTOM) {
        baseline += (int32_t) bounds.height - font->line_height;
    } else if (vertical_align == FONT_ALIGN_MIDDLE_BODY) {
        baseline += ((int32_t) bounds.height - font->ascent) / 2;
    }

    const int32_t bounds_right = (int32_t) bounds.x + bounds.width;
    const int32_t bounds_bottom = (int32_t) bounds.y + bounds.height;
    cursor_x = cursor_start;

    for (uint16_t index = 0U; index < length; index++) {
        const glyph_t *glyph = font_glyph(font, text[index]);
        if (glyph == NULL) {
            continue;
        }

        const int32_t glyph_x = cursor_x + glyph->x_offset;
        const int32_t glyph_y = baseline + glyph->y_offset;
        const uint16_t glyph_stride = (uint16_t) ((glyph->width + 7U) / 8U);
        for (uint16_t row = 0U; row < glyph->height; row++) {
            const int32_t pixel_y = glyph_y + row;
            if ((pixel_y < bounds.y) || (pixel_y >= bounds_bottom) || (pixel_y > UINT16_MAX)) {
                continue;
            }
            for (uint16_t column = 0U; column < glyph->width; column++) {
                const int32_t pixel_x = glyph_x + column;
                if ((pixel_x < bounds.x) || (pixel_x >= bounds_right) || (pixel_x > UINT16_MAX)) {
                    continue;
                }
                const uint8_t source = font->bitmap[glyph->data_offset +
                                                     ((uint32_t) row * glyph_stride) +
                                                     (column / 8U)];
                if ((source & (uint8_t) (0x80U >> (column & 7U))) != 0U) {
                    set_pixel(context, (uint16_t) pixel_x, (uint16_t) pixel_y, colour);
                }
            }
        }

        cursor_x += glyph->x_advance;
    }
}
