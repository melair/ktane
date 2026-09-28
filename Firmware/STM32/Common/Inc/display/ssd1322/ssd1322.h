#ifndef SSD1322_H
#define SSD1322_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "fsm/fsm.h"
#include "spi/spi.h"
#include "sys/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SSD1322_WIDTH 256U
#define SSD1322_HEIGHT 64U
#define SSD1322_FRAMEBUFFER_SIZE (SSD1322_WIDTH * SSD1322_HEIGHT / 2U)

typedef uint8_t SSD1322_Colour;
#define SSD1322_COLOUR_BLACK 0U
#define SSD1322_COLOUR_WHITE 15U

typedef enum {
    SSD1322_ROTATION_0, SSD1322_ROTATION_90,
    SSD1322_ROTATION_180, SSD1322_ROTATION_270,
} SSD1322_Rotation;

typedef enum {
    /* Set bits paint colour; clear bits paint black. */
    SSD1322_SPRITE_COMPOSITE_SET,
    /* Set bits bitwise-OR colour into the pixel; clear bits preserve it. */
    SSD1322_SPRITE_COMPOSITE_OR,
} SSD1322_SpriteComposite;

typedef struct {
    uint16_t x, y, width, height;
} SSD1322_Window;

typedef struct {
    /* 8192 bytes, row-major: even x in high nibble, odd x in low nibble.
     * Keep storage alive and unmodified until IsReady; no double buffering. */
    uint8_t *framebuffer;
    size_t framebuffer_size;
    SPI_Baud baud;
    void *cs_port;
    uint32_t cs_pin;
    GPIO_PinDef dc;
    GPIO_PinDef reset;
    /* Column units are four pixels. Rows must fit the 128-row controller RAM. */
    uint8_t column_offset;
    uint8_t row_offset;
    bool dual_com;
    bool com_split;
    bool com_reverse;
} SSD1322_Config;

typedef struct {
    SSD1322_Config config;
    FSM fsm;
    SPI_Sequence sequence;
    SPI_SequenceStep steps[24];
    SPI_Transaction spi_template;
    SSD1322_Rotation rotation;
    uint8_t remap_data[2];
    uint8_t row_data[2];
    uint8_t column_data[2];
    uint8_t sequence_complete_state;
    bool initialized;
    bool initial_clear;
} SSD1322;

/* Zero-initialize the instance before first use. Do not reinitialize while busy.
 * Init clears the framebuffer and starts asynchronous reset/configuration.
 * Module power must already be stable; its power sequencing is external.
 * Uses internal VDD/VSL and default analogue settings. Default GS0/GS1 both
 * have zero drive brightness. Call Service regularly, alongside SPI_Service. */
bool SSD1322_Init(SSD1322 *display, const SSD1322_Config *config);
void SSD1322_Service(SSD1322 *display);
bool SSD1322_IsReady(const SSD1322 *display);
bool SSD1322_HasError(const SSD1322 *display);
bool SSD1322_Refresh(SSD1322 *display);

/* Drawing and rotation are accepted only when ready. Invalid colours and
 * out-of-bounds pixels are ignored. Rotation does not rearrange existing RAM. */
bool SSD1322_SetRotation(SSD1322 *display, SSD1322_Rotation rotation);
uint16_t SSD1322_Width(const SSD1322 *display);
uint16_t SSD1322_Height(const SSD1322 *display);
void SSD1322_SetPixel(SSD1322 *display, uint16_t x, uint16_t y, uint8_t colour);
void SSD1322_Fill(SSD1322 *display, uint16_t x, uint16_t y, uint16_t width,
                  uint16_t height, SSD1322_Colour colour);
void SSD1322_CopySprite(SSD1322 *display, uint16_t x, uint16_t y, const uint8_t *sprite,
                        uint16_t width, uint16_t height, SSD1322_Colour colour,
                        SSD1322_SpriteComposite composite);
void SSD1322_DrawTestCard(SSD1322 *display);

#ifdef __cplusplus
}
#endif
#endif // SSD1322_H
