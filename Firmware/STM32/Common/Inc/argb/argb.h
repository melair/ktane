#ifndef ARGB_H
#define ARGB_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t r, g, b;
} ARGB_LED;

typedef struct ARGB_Strip {
    uint8_t count;
    uint8_t colour_order;
    uint8_t max_brightness;
    ARGB_LED *leds;
    bool dirty;
    struct ARGB_Strip *next;
} ARGB_Strip;

bool ARGB_Init(void);

void ARGB_Set(ARGB_Strip *strip, uint8_t idx, uint8_t r, uint8_t g, uint8_t b);

void ARGB_Fill(ARGB_Strip *strip, uint8_t r, uint8_t g, uint8_t b);

void ARGB_Set_Brightness(uint8_t max_brightness);

void ARGB_Add_Strip(ARGB_Strip *strip);

void ARGB_Service(void);

#define BITS_PER_COLOUR 8U
#define COLOURS_PER_LED 3U
#define BITS_PER_LED (BITS_PER_COLOUR * COLOURS_PER_LED)
#define LEDS_IN_TRANSPORT_BUFFER 2U

#define ARGB_BRIGHTNESS_MAX UINT8_MAX

#define COLOUR_RED 0x00U
#define COLOUR_GREEN 0x01U
#define COLOUR_BLUE 0x02U

/* Colour components are shifted out in the order encoded from least to most significant. */
#define COLOUR_ORDER_RGB ((COLOUR_BLUE << 4U) | (COLOUR_GREEN << 2U) | COLOUR_RED)
#define COLOUR_ORDER_GRB ((COLOUR_BLUE << 4U) | (COLOUR_RED << 2U) | COLOUR_GREEN)

#ifdef __cplusplus
}
#endif

#endif // ARGB_H
