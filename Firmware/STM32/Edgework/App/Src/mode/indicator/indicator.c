#include "mode/indicator/indicator.h"

#include "mode.h"
#include "main.h"
#include "sys/gpio.h"
#include "sys/spi_config.h"

#include <stddef.h>

#define EPAPER_BUSY_Pin GPIO9_Pin
#define EPAPER_BUSY_Port GPIO9_Port
#define EPAPER_RESET_Pin GPIO10_Pin
#define EPAPER_RESET_Port GPIO10_Port
#define EPAPER_DC_Pin GPIO7_Pin
#define EPAPER_DC_Port GPIO7_Port
#define EPAPER_CS_Pin GPIO5_Pin
#define EPAPER_CS_Port GPIO5_Port

static Indicator_Data *const indicator = &mode_data.mode.indicator;

static void indicator_service(void) {
    SPI_Service();
    Epaper_Service(&indicator->epaper.display);
}

static void indicator_startup_enter(FSM *fsm) {
    (void) fsm;
}

static void indicator_startup_service(FSM *fsm) {
    Epaper *const display = &indicator->epaper.display;
    if (!Epaper_IsReady(display)) {
        return;
    }

    Epaper_Fill(display, 0U, 0U, Epaper_Width(display), Epaper_Height(display),
                EPAPER_COLOUR_WHITE);
    if (Epaper_Refresh(display)) {
        (void) FSM_Transition(fsm, EDGEWORK_MODE_STATE_IDLE);
    }
}

static void indicator_init_enter(FSM *fsm) {
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

    const Epaper_Config epaper_config = {
        .width = INDICATOR_EPAPER_WIDTH,
        .height = INDICATOR_EPAPER_HEIGHT,
        .window = {
            .x = 0U,
            .y = 0U,
            .width = INDICATOR_EPAPER_WIDTH,
            .height = INDICATOR_EPAPER_HEIGHT,
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
};

Mode_Definition indicator_mode = {
    .state_callbacks = indicator_state_callbacks,
    .always_service = NULL,
};
