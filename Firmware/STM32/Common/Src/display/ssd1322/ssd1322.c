#include "display/ssd1322/ssd1322.h"

#include <string.h>

typedef enum {
    DISPLAY_POWER_WAIT,
    DISPLAY_RESET_ASSERT,
    DISPLAY_RESET_RELEASE,
    DISPLAY_CONFIGURE,
    DISPLAY_WINDOW,
    DISPLAY_WRITE,
    DISPLAY_ON,
    DISPLAY_READY,
    DISPLAY_ERROR,
} Display_State;

static void power_wait(FSM *fsm);
static void reset_assert(FSM *fsm);
static void reset_release(FSM *fsm);
static void configure(FSM *fsm);
static void window_send(FSM *fsm);
static void framebuffer_send(FSM *fsm);
static void display_on(FSM *fsm);
static void display_on_wait(FSM *fsm);

static const FSM_State states[] = {
    [DISPLAY_POWER_WAIT] = {.enter = power_wait, .next_mask = FSM_NEXT(DISPLAY_RESET_ASSERT)},
    [DISPLAY_RESET_ASSERT] = {.enter = reset_assert, .next_mask = FSM_NEXT(DISPLAY_RESET_RELEASE)},
    [DISPLAY_RESET_RELEASE] = {.enter = reset_release, .next_mask = FSM_NEXT(DISPLAY_CONFIGURE)},
    [DISPLAY_CONFIGURE] = {.enter = configure,
        .next_mask = FSM_NEXT(DISPLAY_WINDOW) | FSM_NEXT(DISPLAY_ERROR)},
    [DISPLAY_WINDOW] = {.enter = window_send,
        .next_mask = FSM_NEXT(DISPLAY_WRITE) | FSM_NEXT(DISPLAY_ERROR)},
    [DISPLAY_WRITE] = {.enter = framebuffer_send,
        .next_mask = FSM_NEXT(DISPLAY_ON) | FSM_NEXT(DISPLAY_READY) | FSM_NEXT(DISPLAY_ERROR)},
    [DISPLAY_ON] = {.enter = display_on,
        .next_mask = FSM_NEXT(DISPLAY_READY) | FSM_NEXT(DISPLAY_ERROR)},
    [DISPLAY_READY] = {.next_mask = FSM_NEXT(DISPLAY_WINDOW)},
    [DISPLAY_ERROR] = {0},
};

/* Command bytes and arguments have static lifetime for asynchronous SPI. */
static const uint8_t cmd_column = 0x15;
static const uint8_t cmd_row = 0x75;
static const uint8_t cmd_write = 0x5c;
static const uint8_t cmd_on = 0xaf;

static void dc_command(void *context) {
    SSD1322 *display = context;
    HAL_GPIO_WritePin(display->config.dc.port, display->config.dc.pin, GPIO_PIN_RESET);
}

static void dc_data(void *context) {
    SSD1322 *display = context;
    HAL_GPIO_WritePin(display->config.dc.port, display->config.dc.pin, GPIO_PIN_SET);
}

static void sequence_complete(void *context, bool success) {
    SSD1322 *display = context;
    if (!success) {
        FSM_Transition(&display->fsm, DISPLAY_ERROR);
    } else if (display->fsm.current_id == DISPLAY_ON) {
        display_on_wait(&display->fsm);
    } else {
        FSM_Transition(&display->fsm, display->sequence_complete_state);
    }
}


static void queue(SSD1322 *display, uint16_t count, Display_State next) {
    display->sequence_complete_state = next;
    if (SPI_Sequence_Init(&display->sequence, &display->spi_template,
                          display->steps, count, display, sequence_complete)) {
        SPI_Transaction *tx = SPI_Sequence_Start(&display->sequence);
        if (tx != NULL) {
            SPI_Queue(tx);
            return;
        }
    }
    FSM_Transition(&display->fsm, DISPLAY_ERROR);
}

static void power_wait(FSM *fsm) {
    FSM_TransitionIn(fsm, DISPLAY_RESET_ASSERT, 300U);
}

static void reset_assert(FSM *fsm) {
    SSD1322 *display = fsm->context;
    HAL_GPIO_WritePin(display->config.reset.port, display->config.reset.pin, GPIO_PIN_RESET);
    FSM_TransitionIn(fsm, DISPLAY_RESET_RELEASE, 10U);
}

static void reset_release(FSM *fsm) {
    SSD1322 *display = fsm->context;
    HAL_GPIO_WritePin(display->config.reset.port, display->config.reset.pin, GPIO_PIN_SET);
    FSM_TransitionIn(fsm, DISPLAY_CONFIGURE, 10U);
}

static void configure(FSM *fsm) {
    SSD1322 *display = fsm->context;
    static const uint8_t unlock = 0xfd, unlock_data = 0x12;
    static const uint8_t off = 0xae;
    /* Default internal oscillator frequency, clock divider = 2. */
    static const uint8_t clock = 0xb3, clock_data = 0x51;
    static const uint8_t mux = 0xca, mux_data = 0x3f;
    static const uint8_t remap = 0xa0;
    static const uint8_t start = 0xa1;
    static const uint8_t offset = 0xa2, zero = 0;
    static const uint8_t function = 0xab, internal_vdd = 1;
    static const uint8_t normal = 0xa6, full = 0xa9, gray = 0xb9;
    uint16_t n = 0;

    display->steps[n++] = (SPI_SequenceStep){
        .data = &unlock, .size = 1, .prepare = dc_command,
    };
    display->steps[n++] = (SPI_SequenceStep){
        .data = &unlock_data, .size = 1, .prepare = dc_data,
    };
    display->steps[n++] = (SPI_SequenceStep){
        .data = &off, .size = 1, .prepare = dc_command,
    };
    display->steps[n++] = (SPI_SequenceStep){
        .data = &clock, .size = 1, .prepare = dc_command,
    };
    display->steps[n++] = (SPI_SequenceStep){
        .data = &clock_data, .size = 1, .prepare = dc_data,
    };
    display->steps[n++] = (SPI_SequenceStep){
        .data = &mux, .size = 1, .prepare = dc_command,
    };
    display->steps[n++] = (SPI_SequenceStep){
        .data = &mux_data, .size = 1, .prepare = dc_data,
    };
    display->steps[n++] = (SPI_SequenceStep){
        .data = &remap, .size = 1, .prepare = dc_command,
    };
    display->steps[n++] = (SPI_SequenceStep){
        .data = display->remap_data, .size = 2, .prepare = dc_data,
    };
    display->steps[n++] = (SPI_SequenceStep){
        .data = &start, .size = 1, .prepare = dc_command,
    };
    display->steps[n++] = (SPI_SequenceStep){
        .data = &display->config.row_offset, .size = 1, .prepare = dc_data,
    };
    display->steps[n++] = (SPI_SequenceStep){
        .data = &offset, .size = 1, .prepare = dc_command,
    };
    display->steps[n++] = (SPI_SequenceStep){
        .data = &zero, .size = 1, .prepare = dc_data,
    };
    display->steps[n++] = (SPI_SequenceStep){
        .data = &function, .size = 1, .prepare = dc_command,
    };
    display->steps[n++] = (SPI_SequenceStep){
        .data = &internal_vdd, .size = 1, .prepare = dc_data,
    };
    display->steps[n++] = (SPI_SequenceStep){
        .data = &normal, .size = 1, .prepare = dc_command,
    };
    display->steps[n++] = (SPI_SequenceStep){
        .data = &full, .size = 1, .prepare = dc_command,
    };
    display->steps[n++] = (SPI_SequenceStep){
        .data = &gray, .size = 1, .prepare = dc_command,
    };
    /* Remaining analogue registers retain their documented reset defaults. */
    queue(display, n, DISPLAY_WINDOW);
}

static void window_send(FSM *fsm) {
    SSD1322 *display = fsm->context;
    display->steps[0] = (SPI_SequenceStep){
        .data = &cmd_column, .size = 1, .prepare = dc_command,
    };
    display->steps[1] = (SPI_SequenceStep){
        .data = display->column_data, .size = 2, .prepare = dc_data,
    };
    display->steps[2] = (SPI_SequenceStep){
        .data = &cmd_row, .size = 1, .prepare = dc_command,
    };
    display->steps[3] = (SPI_SequenceStep){
        .data = display->row_data, .size = 2, .prepare = dc_data,
    };
    queue(display, 4, DISPLAY_WRITE);
}

static void framebuffer_send(FSM *fsm) {
    SSD1322 *display = fsm->context;
    display->steps[0] = (SPI_SequenceStep){
        .data = &cmd_write, .size = 1, .prepare = dc_command,
    };
    /* STM32H5 SPI4/5/6 have a limited transfer counter: HAL requires fewer
     * than 1023 frames. Keep CS asserted across sixteen 512-byte writes. */
    for (uint16_t index = 0; index < SSD1322_FRAMEBUFFER_SIZE / 512U; index++) {
        display->steps[index + 1U] = (SPI_SequenceStep){
            .data = display->config.framebuffer + index * 512U,
            .size = 512U,
            .prepare = dc_data,
        };
    }
    queue(display, 1U + SSD1322_FRAMEBUFFER_SIZE / 512U,
          display->initial_clear ? DISPLAY_ON : DISPLAY_READY);
}

static void display_on(FSM *fsm) {
    SSD1322 *display = fsm->context;
    display->steps[0] = (SPI_SequenceStep){
        .data = &cmd_on, .size = 1, .prepare = dc_command,
    };
    queue(display, 1, DISPLAY_READY);
}

static void display_on_wait(FSM *fsm) {
    SSD1322 *display = fsm->context;
    display->initial_clear = false;
    FSM_TransitionIn(fsm, DISPLAY_READY, 200U);
}

bool SSD1322_Init(SSD1322 *display, const SSD1322_Config *config) {
    if (display == NULL || config == NULL ||
        (display->initialized && !SSD1322_IsReady(display) && !SSD1322_HasError(display)) ||
        config->framebuffer == NULL || config->framebuffer_size < SSD1322_FRAMEBUFFER_SIZE ||
        config->cs_port == NULL || config->cs_pin == 0 ||
        config->dc.port == NULL || config->dc.pin == 0 ||
        config->reset.port == NULL || config->reset.pin == 0 ||
        (unsigned)config->baud > SPI_BAUD_125KHZ ||
        config->column_offset > 56U || config->row_offset > 64U ||
        (config->dual_com && config->com_split)) {
        return false;
    }

    const SSD1322_Config saved_config = *config;
    memset(display, 0, sizeof(*display));
    display->config = saved_config;
    memset(display->config.framebuffer, 0, SSD1322_FRAMEBUFFER_SIZE);
    display->column_data[0] = config->column_offset;
    display->column_data[1] = config->column_offset + SSD1322_WIDTH / 4U - 1U;
    display->row_data[0] = config->row_offset;
    display->row_data[1] = config->row_offset + SSD1322_HEIGHT - 1U;
    /* Nibble remap makes each byte's high nibble the leftmost pixel. */
    display->remap_data[0] = 0x04U | (config->com_reverse ? 0x10U : 0U) |
                            (config->com_split ? 0x20U : 0U);
    display->remap_data[1] = 0x01U | (config->dual_com ? 0x10U : 0U);
    display->spi_template = (SPI_Transaction){
        .bits = 8, .baud = config->baud, .operation = SPI_OPERATION_WRITE,
        .cs_port = config->cs_port, .cs_pin = config->cs_pin,
        .lsb_first = false, .cke = false, .ckp = false,
    };
    GPIO_InitTypeDef gpio = {
        .Pin = config->cs_pin,
        .Mode = GPIO_MODE_OUTPUT_PP,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_HIGH,
    };
    HAL_GPIO_WritePin(config->cs_port, config->cs_pin, GPIO_PIN_SET);
    HAL_GPIO_Init(config->cs_port, &gpio);

    gpio.Pin = config->dc.pin;
    HAL_GPIO_WritePin(config->dc.port, config->dc.pin, GPIO_PIN_RESET);
    HAL_GPIO_Init(config->dc.port, &gpio);

    gpio.Pin = config->reset.pin;
    HAL_GPIO_WritePin(config->reset.port, config->reset.pin, GPIO_PIN_SET);
    HAL_GPIO_Init(config->reset.port, &gpio);
    display->initial_clear = true;
    display->initialized = FSM_Init(&display->fsm, states, DISPLAY_POWER_WAIT, display);
    return display->initialized;
}

void SSD1322_Service(SSD1322 *display) {
    if (display != NULL && display->initialized) {
        FSM_Service(&display->fsm);
    }
}

bool SSD1322_IsReady(const SSD1322 *display) {
    return display != NULL && display->initialized &&
           display->fsm.current_id == DISPLAY_READY && !display->fsm.transition_pending;
}

bool SSD1322_HasError(const SSD1322 *display) {
    return display != NULL && display->initialized &&
           (display->fsm.current_id == DISPLAY_ERROR ||
            (display->fsm.transition_pending && display->fsm.transition_id == DISPLAY_ERROR));
}

bool SSD1322_Refresh(SSD1322 *display) {
    return SSD1322_IsReady(display) && FSM_Transition(&display->fsm, DISPLAY_WINDOW);
}

bool SSD1322_SetRotation(SSD1322 *display, SSD1322_Rotation rotation) {
    if (!SSD1322_IsReady(display) || (unsigned)rotation > SSD1322_ROTATION_270) {
        return false;
    }
    display->rotation = rotation;
    return true;
}

uint16_t SSD1322_Width(const SSD1322 *display) {
    if (display == NULL) return 0;
    return (display->rotation == SSD1322_ROTATION_90 || display->rotation == SSD1322_ROTATION_270)
        ? SSD1322_HEIGHT : SSD1322_WIDTH;
}

uint16_t SSD1322_Height(const SSD1322 *display) {
    if (display == NULL) return 0;
    return (display->rotation == SSD1322_ROTATION_90 || display->rotation == SSD1322_ROTATION_270)
        ? SSD1322_WIDTH : SSD1322_HEIGHT;
}

static bool map_pixel(const SSD1322 *display, uint16_t x, uint16_t y,
                       uint16_t *px, uint16_t *py) {
    if (!SSD1322_IsReady(display) || x >= SSD1322_Width(display) || y >= SSD1322_Height(display)) {
        return false;
    }
    switch (display->rotation) {
        case SSD1322_ROTATION_0: *px = x; *py = y; break;
        case SSD1322_ROTATION_90: *px = SSD1322_WIDTH - 1U - y; *py = x; break;
        case SSD1322_ROTATION_180:
            *px = SSD1322_WIDTH - 1U - x; *py = SSD1322_HEIGHT - 1U - y; break;
        case SSD1322_ROTATION_270: *px = y; *py = SSD1322_HEIGHT - 1U - x; break;
        default: return false;
    }
    return true;
}

static void paint_pixel(SSD1322 *display, uint16_t x, uint16_t y,
                         SSD1322_Colour colour, bool merge) {
    uint16_t px, py;
    if (colour > 15U || !map_pixel(display, x, y, &px, &py)) return;
    uint8_t *byte = &display->config.framebuffer[py * (SSD1322_WIDTH / 2U) + px / 2U];
    const uint8_t shift = (px & 1U) ? 0U : 4U;
    const uint8_t value = colour << shift;
    if (merge) *byte |= value;
    else *byte = (*byte & (uint8_t)~(0x0fU << shift)) | value;
}

void SSD1322_SetPixel(SSD1322 *display, uint16_t x, uint16_t y, uint8_t colour) {
    paint_pixel(display, x, y, colour, false);
}

static void ssd1322_or_pixel(SSD1322 *display, uint16_t x, uint16_t y, SSD1322_Colour colour) {
    paint_pixel(display, x, y, colour, true);
}

void SSD1322_Fill(SSD1322 *display, uint16_t x, uint16_t y, uint16_t width,
                  uint16_t height, SSD1322_Colour colour) {
    if (!SSD1322_IsReady(display) || colour > 15U ||
        x >= SSD1322_Width(display) || y >= SSD1322_Height(display)) return;
    uint16_t w = SSD1322_Width(display) - x, h = SSD1322_Height(display) - y;
    if (w > width) w = width;
    if (h > height) h = height;
    for (uint16_t row = 0; row < h; row++)
        for (uint16_t col = 0; col < w; col++)
            SSD1322_SetPixel(display, x + col, y + row, colour);
}

void SSD1322_DrawTestCard(SSD1322 *display) {
    if (!SSD1322_IsReady(display)) return;
    uint16_t w = SSD1322_Width(display), h = SSD1322_Height(display);
    SSD1322_Fill(display, 0, 0, w, h, SSD1322_COLOUR_BLACK);
    for (uint16_t x = 0; x < w; x++)
        SSD1322_Fill(display, x, h / 2U, 1, h / 2U, (uint32_t)x * 16U / w);
    for (uint16_t y = 0; y < h / 2U; y++)
        for (uint16_t x = 0; x < w; x++)
            if (x % 10U == 0 || y % 10U == 0)
                SSD1322_SetPixel(display, x, y, SSD1322_COLOUR_WHITE);
    SSD1322_Fill(display, 0, 0, w, 1, 15);
    SSD1322_Fill(display, 0, h - 1U, w, 1, 15);
    SSD1322_Fill(display, 0, 0, 1, h, 15);
    SSD1322_Fill(display, w - 1U, 0, 1, h, 15);
    SSD1322_Fill(display, 1, 1, 3, 3, 15);
}

void SSD1322_CopySprite(SSD1322 *ssd1322, uint16_t x, uint16_t y, const uint8_t *sprite,
                       uint16_t width, uint16_t height, SSD1322_Colour colour,
                       SSD1322_SpriteComposite composite) {
    if (!SSD1322_IsReady(ssd1322) || (sprite == NULL) || (width == 0U) || (height == 0U) ||
        (colour > SSD1322_COLOUR_WHITE) || ((unsigned) composite > SSD1322_SPRITE_COMPOSITE_OR)) {
        return;
    }

    const uint16_t display_width = SSD1322_Width(ssd1322);
    const uint16_t display_height = SSD1322_Height(ssd1322);
    if ((x >= display_width) || (y >= display_height)) {
        return;
    }

    const uint16_t sprite_stride = (uint16_t) ((width + 7U) / 8U);
    for (uint16_t row = 0U; row < height; row++) {
        if (row >= (display_height - y)) {
            break;
        }
        for (uint16_t col = 0U; col < width; col++) {
            if (col >= (display_width - x)) {
                break;
            }
            const uint8_t source = sprite[((uint32_t) row * sprite_stride) + (col / 8U)];
            if ((source & (uint8_t) (0x80U >> (col & 7U))) != 0U) {
                if (composite == SSD1322_SPRITE_COMPOSITE_SET) {
                    SSD1322_SetPixel(ssd1322, (uint16_t) (x + col), (uint16_t) (y + row), colour);
                } else {
                    ssd1322_or_pixel(ssd1322, (uint16_t) (x + col), (uint16_t) (y + row), colour);
                }
            } else if (composite == SSD1322_SPRITE_COMPOSITE_SET) {
                SSD1322_SetPixel(ssd1322, (uint16_t) (x + col), (uint16_t) (y + row),
                                SSD1322_COLOUR_BLACK);
            }
        }
    }
}
