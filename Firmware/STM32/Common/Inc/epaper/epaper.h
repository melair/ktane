#ifndef EPAPER_H
#define EPAPER_H

#include <stdbool.h>
#include <stdint.h>

#include "fsm/fsm.h"
#include "spi/spi.h"
#include "sys/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    EPAPER_COLOUR_WHITE = 0,
    EPAPER_COLOUR_BLACK,
    EPAPER_COLOUR_RED,
} Epaper_Colour;

typedef enum {
    /* Paint source set bits in colour and source clear bits white. */
    EPAPER_SPRITE_COMPOSITE_SET = 0,
    /* Preserve destination pixels for source clear bits and merge source set bits. */
    EPAPER_SPRITE_COMPOSITE_OR,
} Epaper_SpriteComposite;

typedef enum {
    EPAPER_ROTATION_0 = 0,
    EPAPER_ROTATION_90,
    EPAPER_ROTATION_180,
    EPAPER_ROTATION_270,
} Epaper_Rotation;

/* A visible rectangle in the native, unrotated panel coordinate system. */
typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
} Epaper_Window;

typedef struct {
    /* Native controller canvas dimensions. */
    uint16_t width;
    uint16_t height;
    /* The part of the canvas visible through the mechanical bezel. */
    Epaper_Window window;
    uint8_t *black_framebuffer;
    uint8_t *red_framebuffer;

    SPI_Baud baud;
    void *cs_port;
    uint32_t cs_pin;
    GPIO_PinDef dc;
    GPIO_PinDef reset;
    GPIO_PinDef busy;
} Epaper_Config;

typedef struct {
    Epaper_Config config;
    FSM fsm;
    SPI_Sequence sequence;
    SPI_SequenceStep steps[13];
    SPI_Transaction spi_template;
    uint8_t driver_output_data[3];
    uint8_t ram_x_data[2];
    uint8_t ram_y_data[4];
    Epaper_Rotation rotation;
    uint16_t stride;
    uint8_t sequence_complete_state;
} Epaper;

bool Epaper_Init(Epaper *epaper, const Epaper_Config *config);

void Epaper_Service(Epaper *epaper);

bool Epaper_IsReady(const Epaper *epaper);

bool Epaper_Refresh(Epaper *epaper);

bool Epaper_SetRotation(Epaper *epaper, Epaper_Rotation rotation);

uint16_t Epaper_Width(const Epaper *epaper);

uint16_t Epaper_Height(const Epaper *epaper);

void Epaper_SetPixel(Epaper *epaper, uint16_t x, uint16_t y, Epaper_Colour colour);

void Epaper_Fill(Epaper *epaper, uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                 Epaper_Colour colour);

/* Draw a test grid with 10 px solid and 5 px dotted lines, marking logical (0, 0). */
void Epaper_DrawTestCard(Epaper *epaper);

void Epaper_CopySprite(Epaper *epaper, uint16_t x, uint16_t y, const uint8_t *sprite,
                       uint16_t width, uint16_t height, Epaper_Colour colour,
                       Epaper_SpriteComposite composite);

#ifdef __cplusplus
}
#endif

#endif // EPAPER_H
