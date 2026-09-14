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
    serial->display_refresh_started = false;
    serial->displayed_state = mode_data.desired_state;
    const edgework_state_t *const displayed_state = &serial->displayed_state;

    if (displayed_state->identify != 0U) {
        const uint8_t slot = Slot_Get();
        const uint8_t identify_text[] = {
            'I', 'D', '#', ' ',
            serial_hex_digit(slot >> 4U),
            serial_hex_digit(slot),
        };

        Epaper_Fill(display, 0U, 0U, Epaper_Width(display), Epaper_Height(display),
                    EPAPER_COLOUR_WHITE);
        Epaper_Text(display, identify_text, sizeof(identify_text), &anonymous_pro_font,
                    (Epaper_Window) {0U, 0U, Epaper_Width(display), Epaper_Height(display)},
                    EPAPER_TEXT_ALIGN_CENTRE, EPAPER_TEXT_ALIGN_MIDDLE,
                    EPAPER_COLOUR_BLACK);
    } else {
        const uint16_t red_banner = 50U;

        Epaper_Fill(display, 0, 0, Epaper_Width(display), Epaper_Height(display), EPAPER_COLOUR_WHITE);
        Epaper_Fill(display, 0U, 0U, Epaper_Width(display), red_banner, EPAPER_COLOUR_RED);

        if ((serial_label_width <= Epaper_Width(display)) &&
            (serial_label_height <= red_banner)) {
            const uint16_t label_x = (Epaper_Width(display) - serial_label_width) / 2U;
            const uint16_t label_y = (red_banner - serial_label_height) / 2U;
            Epaper_CopySprite(display, label_x, label_y, serial_label_bitmap, serial_label_width,
                              serial_label_height, EPAPER_COLOUR_WHITE,
                              EPAPER_SPRITE_COMPOSITE_OR);
        }

        Epaper_Text(display, displayed_state->serial.value,
                    sizeof(displayed_state->serial.value), &anonymous_pro_font,
                    (Epaper_Window) {0U, red_banner, Epaper_Width(display),
                                     Epaper_Height(display) - red_banner},
                    EPAPER_TEXT_ALIGN_CENTRE, EPAPER_TEXT_ALIGN_MIDDLE,
                    EPAPER_COLOUR_BLACK);
    }

    serial->display_refresh_started = Epaper_Refresh(display);
}

static void serial_clear_enter(FSM *fsm) {
    (void) fsm;

    Epaper *const display = &serial->epaper.display;
    serial->display_refresh_started = false;
    Epaper_Fill(display, 0U, 0U, Epaper_Width(display), Epaper_Height(display),
                EPAPER_COLOUR_WHITE);
    serial->display_refresh_started = Epaper_Refresh(display);
}

static void serial_clear_service(FSM *fsm) {
    Epaper *const display = &serial->epaper.display;
    if (!serial->display_refresh_started) {
        if (Epaper_IsReady(display)) {
            serial->display_refresh_started = Epaper_Refresh(display);
        }
        return;
    }

    if (Epaper_IsReady(display)) {
        (void) FSM_Transition(fsm, EDGEWORK_MODE_STATE_IDLE);
    }
}

static void serial_display_service(FSM *fsm) {
    Epaper *const display = &serial->epaper.display;
    if (!serial->display_refresh_started) {
        if (Epaper_IsReady(display)) {
            serial->display_refresh_started = Epaper_Refresh(display);
        }
        return;
    }

    if (Epaper_IsReady(display)) {
        mode_data.current_state = serial->displayed_state;
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
    [EDGEWORK_MODE_STATE_CLEAR] = {
        .enter = serial_clear_enter,
        .service = serial_clear_service,
    },
    [EDGEWORK_MODE_STATE_DISPLAY] = {
        .enter = serial_display_enter,
        .service = serial_display_service,
    },
};

Mode_Definition serial_mode = {
    .state_callbacks = serial_state_callbacks,
    .always_service = NULL,
    .display_acknowledges_state = true,
};
