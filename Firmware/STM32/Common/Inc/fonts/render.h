#ifndef STM32FIRMWARE_FONT_RENDER_H
#define STM32FIRMWARE_FONT_RENDER_H

#include "font.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t x, y, width, height;
} Font_Bounds;

typedef enum {
    FONT_ALIGN_LEFT = 0,
    FONT_ALIGN_CENTRE,
    FONT_ALIGN_RIGHT,
} Font_HorizontalAlign;

typedef enum {
    FONT_ALIGN_TOP = 0,
    FONT_ALIGN_MIDDLE,
    FONT_ALIGN_BOTTOM,
    /* Centre the ascent, excluding the descent. */
    FONT_ALIGN_MIDDLE_BODY,
} Font_VerticalAlign;

/* The callback handles display clipping, rotation and colour interpretation. */
typedef void (*Font_SetPixel)(void *context, uint16_t x, uint16_t y, uint8_t colour);

/* Render one line synchronously, without retaining context or allocating.
 * Only set bitmap bits are painted; colour is passed through unchanged.
 * Missing characters are skipped without advancing. Pixels are clipped to bounds.
 * Bounds must fit line_height, or ascent for MIDDLE_BODY.
 * Null callback/text/font/bitmap and invalid alignment values draw nothing.
 * Context is opaque and may be NULL if the callback supports it. */
void Font_Text(Font_SetPixel set_pixel, void *context,
               const uint8_t *text, uint16_t length, const font_t *font,
               Font_Bounds bounds, Font_HorizontalAlign horizontal_align,
               Font_VerticalAlign vertical_align, uint8_t colour);

#ifdef __cplusplus
}
#endif

#endif // STM32FIRMWARE_FONT_RENDER_H
