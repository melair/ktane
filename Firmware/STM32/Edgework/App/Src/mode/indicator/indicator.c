#include "mode/indicator/indicator.h"

#include "fonts/ostrich_sans.h"
#include "mode.h"
#include "main.h"
#include "slot.h"
#include "sys/gpio.h"
#include "sys/spi_config.h"

#include <stddef.h>

#define INDICATOR_TEXT_LENGTH 3U
#define INDICATOR_TEXT_COUNT  11U
#define INDICATOR_IDENTIFY_LED_HALF_PERIOD_MS 250U

#define EPAPER_BUSY_Pin GPIO9_Pin
#define EPAPER_BUSY_Port GPIO9_Port
#define EPAPER_RESET_Pin GPIO10_Pin
#define EPAPER_RESET_Port GPIO10_Port
#define EPAPER_DC_Pin GPIO7_Pin
#define EPAPER_DC_Port GPIO7_Port
#define EPAPER_CS_Pin GPIO5_Pin
#define EPAPER_CS_Port GPIO5_Port

static Indicator_Data *const indicator = &mode_data.mode.indicator;

static const uint8_t indicator_text[INDICATOR_TEXT_COUNT][INDICATOR_TEXT_LENGTH] = {
    {'B', 'O', 'B'},
    {'C', 'A', 'R'},
    {'C', 'L', 'R'},
    {'F', 'R', 'K'},
    {'F', 'R', 'Q'},
    {'I', 'N', 'D'},
    {'M', 'S', 'A'},
    {'N', 'S', 'A'},
    {'S', 'I', 'G'},
    {'S', 'N', 'D'},
    {'T', 'R', 'N'},
};

static uint8_t indicator_hex_digit(uint8_t value) {
    value &= 0x0fU;
    return value < 10U ? (uint8_t) ('0' + value) : (uint8_t) ('A' + value - 10U);
}

static void indicator_set_led(const bool lit) {
    HAL_GPIO_WritePin(GPIO11_Port, GPIO11_Pin, lit ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void indicator_service(void) {
    SPI_Service();
    Epaper_Service(&indicator->epaper.display);

    if (mode_data.desired_state.identify != 0U) {
        const bool led_on =
            ((HAL_GetTick() / INDICATOR_IDENTIFY_LED_HALF_PERIOD_MS) & 1U) == 0U;
        indicator_set_led(led_on);
    } else {
        indicator_set_led(mode_data.desired_state.indicator.flags.lit != 0U);
    }
}

static void indicator_startup_enter(FSM *fsm) {
    (void) fsm;
    indicator->startup_refresh_started = false;
}

static void indicator_startup_service(FSM *fsm) {
    Epaper *const display = &indicator->epaper.display;
    if (!Epaper_IsReady(display)) {
        return;
    }

    if (indicator->startup_refresh_started) {
        if (Epaper_IsReady(display)) {
            (void) FSM_Transition(fsm, EDGEWORK_MODE_STATE_IDLE);
        }
        return;
    }

    Epaper_SetRotation(display, EPAPER_ROTATION_90);
    Epaper_Fill(display, 0U, 0U, Epaper_Width(display), Epaper_Height(display),
                EPAPER_COLOUR_BLACK);
    indicator->startup_refresh_started = Epaper_Refresh(display);
}

static void indicator_display_enter(FSM *fsm) {
    (void) fsm;

    Epaper *const display = &indicator->epaper.display;
    indicator->display_refresh_started = false;
    indicator->displayed_state = mode_data.desired_state;
    const edgework_state_t *const displayed_state = &indicator->displayed_state;

    indicator_set_led(displayed_state->indicator.flags.lit != 0U);
    Epaper_Fill(display, 0U, 0U, Epaper_Width(display), Epaper_Height(display),
                EPAPER_COLOUR_BLACK);
    if (displayed_state->identify != 0U) {
        const uint8_t slot = Slot_Get();
        const uint8_t identify_text[] = {
            'S',
            indicator_hex_digit(slot >> 4U),
            indicator_hex_digit(slot),
        };

        Epaper_Text(display, identify_text, sizeof(identify_text), &ostrich_sans_font,
                    (Epaper_Window) {0U, 0U, Epaper_Width(display), Epaper_Height(display)},
                    EPAPER_TEXT_ALIGN_CENTRE, EPAPER_TEXT_ALIGN_MIDDLE_BODY, EPAPER_COLOUR_WHITE);
    } else if ((displayed_state->indicator.indicator > 0U) &&
        (displayed_state->indicator.indicator <= INDICATOR_TEXT_COUNT)) {
        Epaper_Text(display, indicator_text[displayed_state->indicator.indicator - 1U],
                    INDICATOR_TEXT_LENGTH, &ostrich_sans_font,
                    (Epaper_Window) {0U, 0U, Epaper_Width(display), Epaper_Height(display)},
                    EPAPER_TEXT_ALIGN_CENTRE, EPAPER_TEXT_ALIGN_MIDDLE_BODY, EPAPER_COLOUR_WHITE);
    }

    indicator->display_refresh_started = Epaper_Refresh(display);
}

static void indicator_display_service(FSM *fsm) {
    Epaper *const display = &indicator->epaper.display;
    if (!indicator->display_refresh_started) {
        if (Epaper_IsReady(display)) {
            indicator->display_refresh_started = Epaper_Refresh(display);
        }
        return;
    }

    if (Epaper_IsReady(display)) {
        mode_data.current_state = indicator->displayed_state;
        (void) FSM_Transition(fsm, EDGEWORK_MODE_STATE_IDLE);
    }
}

static void indicator_init_enter(FSM *fsm) {
    HAL_GPIO_WritePin(GPIO11_Port, GPIO11_Pin, GPIO_PIN_RESET);
    GPIO_InitTypeDef led_gpio_init = {
        .Pin = GPIO11_Pin,
        .Mode = GPIO_MODE_OUTPUT_PP,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_LOW,
    };
    HAL_GPIO_Init(GPIO11_Port, &led_gpio_init);

    HAL_GPIO_WritePin(EPAPER_CS_Port, EPAPER_CS_Pin, GPIO_PIN_SET);
    GPIO_InitTypeDef gpio_init = {
        .Pin = EPAPER_CS_Pin,
        .Mode = GPIO_MODE_OUTPUT_OD,
        .Pull = GPIO_PULLUP,
        .Speed = GPIO_SPEED_FREQ_LOW,
    };
    HAL_GPIO_Init(EPAPER_CS_Port, &gpio_init);

    SPI_Config(&indicator->spi.handle, &indicator->spi.dma_handle);
    if (!SPI_Init()) {
        Error_Handler();
        return;
    }

#define INDICATOR_EPAPER_TRIM_LEFT 12
#define INDICATOR_EPAPER_TRIM_RIGHT 10
#define INDICATOR_EPAPER_TRIM_TOP 16
#define INDICATOR_EPAPER_TRIM_BOTTOM 32

    const Epaper_Config epaper_config = {
        .width = INDICATOR_EPAPER_WIDTH,
        .height = INDICATOR_EPAPER_HEIGHT,
        .window = {
            .x = INDICATOR_EPAPER_TRIM_LEFT,
            .y = INDICATOR_EPAPER_TRIM_TOP,
            .width = INDICATOR_EPAPER_WIDTH-INDICATOR_EPAPER_TRIM_LEFT-INDICATOR_EPAPER_TRIM_RIGHT,
            .height = INDICATOR_EPAPER_HEIGHT-INDICATOR_EPAPER_TRIM_TOP-INDICATOR_EPAPER_TRIM_BOTTOM,
        },
        .black_framebuffer = indicator->epaper.black,
        .red_framebuffer = NULL,
        .baud = SPI_BAUD_1MHZ,
        .cs_port = EPAPER_CS_Port,
        .cs_pin = EPAPER_CS_Pin,
        .dc = {EPAPER_DC_Port, EPAPER_DC_Pin},
        .reset = {EPAPER_RESET_Port, EPAPER_RESET_Pin},
        .busy = {EPAPER_BUSY_Port, EPAPER_BUSY_Pin},
    };
    if (!Epaper_Init(&indicator->epaper.display, &epaper_config)) {
        Error_Handler();
        return;
    }

    indicator_mode.always_service = indicator_service;
    (void) FSM_Transition(fsm, EDGEWORK_MODE_STATE_STARTUP);
}

static Mode_Callbacks indicator_state_callbacks[EDGEWORK_MODE_STATE_COUNT] = {
    [EDGEWORK_MODE_STATE_INIT] = {
        .enter = indicator_init_enter,
    },
    [EDGEWORK_MODE_STATE_STARTUP] = {
        .enter = indicator_startup_enter,
        .service = indicator_startup_service,
    },
    [EDGEWORK_MODE_STATE_DISPLAY] = {
        .enter = indicator_display_enter,
        .service = indicator_display_service,
    },
};

Mode_Definition indicator_mode = {
    .state_callbacks = indicator_state_callbacks,
    .always_service = NULL,
    .display_acknowledges_state = true,
};
