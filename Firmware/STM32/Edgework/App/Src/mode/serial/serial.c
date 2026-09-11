#include "mode/serial/serial.h"

#include "fonts/anonymous_pro.h"
#include "mode.h"
#include "main.h"
#include "slot.h"
#include "sys/gpio.h"
#include "sys/spi_config.h"

#include <stddef.h>
#include <string.h>

#define EPAPER_BUSY_Pin GPIO9_Pin
#define EPAPER_BUSY_Port GPIO9_Port
#define EPAPER_RESET_Pin GPIO10_Pin
#define EPAPER_RESET_Port GPIO10_Port
#define EPAPER_DC_Pin GPIO7_Pin
#define EPAPER_DC_Port GPIO7_Port
#define EPAPER_CS_Pin GPIO5_Pin
#define EPAPER_CS_Port GPIO5_Port

static Serial_Data *const serial = &mode_data.mode.serial;

static const glyph_t *serial_glyph(const uint8_t character) {
    if (character >= (sizeof(anonymous_pro_ascii_map) / sizeof(anonymous_pro_ascii_map[0]))) {
        return NULL;
    }

    const int16_t index = anonymous_pro_ascii_map[character];
    if ((index < 0) || ((uint16_t) index >= anonymous_pro_glyph_count)) {
        return NULL;
    }

    return &anonymous_pro_glyphs[index];
}

static bool serial_text_bounds(const uint8_t *text, uint16_t length,
                               int32_t *left, int32_t *right) {
    if ((text == NULL) || (left == NULL) || (right == NULL)) {
        return false;
    }

    bool has_glyph = false;
    int32_t cursor_x = 0;
    for (uint16_t index = 0U; index < length; index++) {
        const glyph_t *glyph = serial_glyph(text[index]);
        if (glyph != NULL) {
            const int32_t glyph_left = cursor_x + glyph->x_offset;
            const int32_t glyph_right = glyph_left + glyph->width;
            if (!has_glyph || (glyph_left < *left)) {
                *left = glyph_left;
            }
            if (!has_glyph || (glyph_right > *right)) {
                *right = glyph_right;
            }
            has_glyph = true;
            cursor_x += glyph->x_advance;
        }
    }

    return has_glyph;
}

static void serial_draw_text(Epaper *display, const uint8_t *text, uint16_t length,
                             uint16_t region_y, uint16_t region_height) {
    if ((display == NULL) || (text == NULL) || (region_height < anonymous_pro_line_height)) {
        return;
    }

    int32_t text_left;
    int32_t text_right;
    if (!serial_text_bounds(text, length, &text_left, &text_right)) {
        return;
    }

    const int32_t cursor_start = ((int32_t) Epaper_Width(display) -
                                  (text_right - text_left)) / 2 - text_left;
    const int32_t baseline = region_y +
                             ((region_height - anonymous_pro_line_height) / 2U) +
                             anonymous_pro_ascent;
    int32_t cursor_x = cursor_start;

    for (uint16_t index = 0U; index < length; index++) {
        const glyph_t *glyph = serial_glyph(text[index]);
        if (glyph == NULL) {
            continue;
        }

        const int32_t glyph_x = cursor_x + glyph->x_offset;
        const int32_t glyph_y = baseline + glyph->y_offset;
        if ((glyph_x >= 0) && (glyph_y >= 0)) {
            Epaper_CopySprite(display, (uint16_t) glyph_x, (uint16_t) glyph_y,
                              &anonymous_pro_bitmap[glyph->data_offset], glyph->width,
                              glyph->height, EPAPER_COLOUR_BLACK,
                              EPAPER_SPRITE_COMPOSITE_OR);
        }

        cursor_x += glyph->x_advance;
    }
}

static bool serial_value_is_blank(void) {
    for (uint16_t index = 0U; index < sizeof(mode_data.desired_state.serial.value); index++) {
        if (mode_data.desired_state.serial.value[index] != ' ') {
            return false;
        }
    }

    return true;
}

static uint8_t serial_hex_digit(uint8_t value) {
    value &= 0x0fU;
    return value < 10U ? (uint8_t) ('0' + value) : (uint8_t) ('A' + value - 10U);
}

static void serial_service(void) {
    SPI_Service();
    Epaper_Service(&serial->epaper.display);
}

static void serial_startup_enter(FSM *fsm) {
    (void) fsm;
    serial->startup_refresh_started = false;
}

static void serial_startup_service(FSM *fsm) {
    Epaper *const display = &serial->epaper.display;
    if (serial->startup_refresh_started) {
        if (Epaper_IsReady(display)) {
            (void) FSM_Transition(fsm, EDGEWORK_MODE_STATE_IDLE);
        }
        return;
    }

    if (!Epaper_IsReady(display)) {
        return;
    }

    Epaper_SetRotation(&serial->epaper.display, EPAPER_ROTATION_90);
    Epaper_Fill(display, 0, 0, Epaper_Width(display), Epaper_Height(display), EPAPER_COLOUR_WHITE);

    serial->startup_refresh_started = Epaper_Refresh(display);
}

static void serial_display_enter(FSM *fsm) {
    (void) fsm;

    Epaper *display = &serial->epaper.display;

    if (mode_data.desired_state.identify != 0U) {
        const uint8_t slot = Slot_Get();
        const uint8_t identify_text[] = {
            'I', 'D', '#', ' ',
            serial_hex_digit(slot >> 4U),
            serial_hex_digit(slot),
        };

        Epaper_Fill(display, 0U, 0U, Epaper_Width(display), Epaper_Height(display),
                    EPAPER_COLOUR_WHITE);
        serial_draw_text(display, identify_text, sizeof(identify_text), 0U,
                         Epaper_Height(display));
    } else if (!serial_value_is_blank()) {
        const uint16_t red_banner = 50U;

        Epaper_Fill(display, 0U, 0U, Epaper_Width(display), red_banner, EPAPER_COLOUR_RED);

        if ((serial_label_width <= Epaper_Width(display)) &&
            (serial_label_height <= red_banner)) {
            const uint16_t label_x = (Epaper_Width(display) - serial_label_width) / 2U;
            const uint16_t label_y = (red_banner - serial_label_height) / 2U;
            Epaper_CopySprite(display, label_x, label_y, serial_label_bitmap, serial_label_width,
                              serial_label_height, EPAPER_COLOUR_WHITE,
                              EPAPER_SPRITE_COMPOSITE_OR);
        }

        serial_draw_text(display, mode_data.desired_state.serial.value,
                         sizeof(mode_data.desired_state.serial.value), red_banner,
                         Epaper_Height(display) - red_banner);
    } else {
        Epaper_Fill(display, 0U, 0U, Epaper_Width(display), Epaper_Height(display), EPAPER_COLOUR_WHITE);
    }

    Epaper_Refresh(display);
}

static void serial_display_service(FSM *fsm) {
    if (Epaper_IsReady(&serial->epaper.display)) {
        mode_data.current_state = mode_data.desired_state;
        (void) FSM_Transition(fsm, EDGEWORK_MODE_STATE_IDLE);
    }
}

static void serial_init_enter(FSM *fsm) {
    HAL_GPIO_WritePin(EPAPER_CS_Port, EPAPER_CS_Pin, GPIO_PIN_SET);
    GPIO_InitTypeDef gpio_init = {
        .Pin = EPAPER_CS_Pin,
        /* Release CS high; only pull it low while a transaction is active. */
        .Mode = GPIO_MODE_OUTPUT_OD,
        .Pull = GPIO_PULLUP,
        .Speed = GPIO_SPEED_FREQ_LOW,
    };
    HAL_GPIO_Init(EPAPER_CS_Port, &gpio_init);

    SPI_Config(&serial->spi.handle, &serial->spi.dma_handle);
    if (!SPI_Init()) {
        Error_Handler();
        return;
    }

    const Epaper_Config epaper_config = {
        .width = SERIAL_EPAPER_WIDTH,
        .height = SERIAL_EPAPER_HEIGHT,
        .window = {
            .x = 4U,
            .y = 3U,
            .width = SERIAL_EPAPER_WIDTH-4-4,
            .height = SERIAL_EPAPER_HEIGHT-3-6,
        },
        .black_framebuffer = serial->epaper.black,
        .red_framebuffer = serial->epaper.red,
        .baud = SPI_BAUD_1MHZ,
        .cs_port = EPAPER_CS_Port,
        .cs_pin = EPAPER_CS_Pin,
        .dc = {EPAPER_DC_Port, EPAPER_DC_Pin},
        .reset = {EPAPER_RESET_Port, EPAPER_RESET_Pin},
        .busy = {EPAPER_BUSY_Port, EPAPER_BUSY_Pin},
    };
    if (!Epaper_Init(&serial->epaper.display, &epaper_config)) {
        Error_Handler();
        return;
    }

    serial_mode.always_service = serial_service;
    (void) FSM_Transition(fsm, EDGEWORK_MODE_STATE_STARTUP);
}

static Mode_Callbacks serial_state_callbacks[EDGEWORK_MODE_STATE_COUNT] = {
    [EDGEWORK_MODE_STATE_INIT] = {
        .enter = serial_init_enter,
    },
    [EDGEWORK_MODE_STATE_STARTUP] = {
        .enter = serial_startup_enter,
        .service = serial_startup_service,
    },
    [EDGEWORK_MODE_STATE_DISPLAY] = {
        .enter = serial_display_enter,
        .service = serial_display_service,
    },
};

Mode_Definition serial_mode = {
    .state_callbacks = serial_state_callbacks,
    .always_service = NULL,
};
