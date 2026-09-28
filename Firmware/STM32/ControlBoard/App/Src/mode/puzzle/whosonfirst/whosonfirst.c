#include "mode/puzzle/whosonfirst/whosonfirst.h"
#include "mode.h"
#include "mode_fsm.h"
#include "sys/gpio.h"
#include <stddef.h>

#define VFD_EN_Pin GPIO_A7_Pin
#define VFD_EN_Port GPIO_A7_Port
#define VFD_RESET_Pin GPIO_A6_Pin
#define VFD_RESET_Port GPIO_A6_Port
#define VFD_CS_Pin GPIO_A5_Pin
#define VFD_CS_Port GPIO_A5_Port

#define EPAPER_CS_Pin GPIO_B7_Pin
#define EPAPER_CS_Port GPIO_B7_Port
#define EPAPER_DC_Pin GPIO_B6_Pin
#define EPAPER_DC_Port GPIO_B6_Port
#define EPAPER_RESET_Pin GPIO_B5_Pin
#define EPAPER_RESET_Port GPIO_B5_Port
#define EPAPER_BUSY_Pin GPIO_B4_Pin
#define EPAPER_BUSY_Port GPIO_B4_Port

#define TOUCH_RESET_Pin GPIO_C7_Pin
#define TOUCH_RESET_Port GPIO_C7_Port
#define TOUCH_INT_Pin GPIO_C6_Pin
#define TOUCH_INT_Port GPIO_C6_Port
#define TOUCH_I2C_ADDRESS 0x38U

static WhosOnFirst_Data *const whosonfirst = &mode_data.mode.whosonfirst;

static void whosonfirst_init_enter(FSM *fsm) {
    const VFD_Config vfd_config = {
        .enable = {VFD_EN_Port, VFD_EN_Pin},
        .reset = {VFD_RESET_Port, VFD_RESET_Pin},
        .cs = {VFD_CS_Port, VFD_CS_Pin},
    };
    (void) VFD_Init(&whosonfirst->vfd, &vfd_config);

    HAL_GPIO_WritePin(EPAPER_CS_Port, EPAPER_CS_Pin, GPIO_PIN_SET);
    GPIO_InitTypeDef epaper_cs_gpio = {
        .Pin = EPAPER_CS_Pin,
        .Mode = GPIO_MODE_OUTPUT_OD,
        .Pull = GPIO_PULLUP,
        .Speed = GPIO_SPEED_FREQ_LOW,
    };
    HAL_GPIO_Init(EPAPER_CS_Port, &epaper_cs_gpio);

    const Epaper_Config epaper_config = {
        .width = WHOSONFIRST_EPAPER_WIDTH,
        .height = WHOSONFIRST_EPAPER_HEIGHT,
        .window = {
            .x = 0U,
            .y = 0U,
            .width = WHOSONFIRST_EPAPER_WIDTH,
            .height = WHOSONFIRST_EPAPER_HEIGHT,
        },
        .black_framebuffer = whosonfirst->epaper.black,
        .red_framebuffer = NULL,
        .controller = EPAPER_CONTROLLER_SSD1683,
        .baud = SPI_BAUD_1MHZ,
        .cs_port = EPAPER_CS_Port,
        .cs_pin = EPAPER_CS_Pin,
        .dc = {EPAPER_DC_Port, EPAPER_DC_Pin},
        .reset = {EPAPER_RESET_Port, EPAPER_RESET_Pin},
        .busy = {EPAPER_BUSY_Port, EPAPER_BUSY_Pin},
    };
    whosonfirst->epaper_initialized = Epaper_Init(&whosonfirst->epaper.display, &epaper_config);

    IM_EventQueue_Clear(&whosonfirst->touch_queue);
    const Touch_Config touch_config = {
        .reset = {TOUCH_RESET_Port, TOUCH_RESET_Pin},
        .interrupt = {TOUCH_INT_Port, TOUCH_INT_Pin},
        .address = TOUCH_I2C_ADDRESS,
        .queue = &whosonfirst->touch_queue,
    };
    (void) Touch_Init(&whosonfirst->touch, &touch_config);

    Mode_SetServiceEnabled(true);
    FSM_Transition(fsm, MODE_FSM_STATE_STARTUP);
}

static void whosonfirst_always_service(void) {
    VFD_Service(&whosonfirst->vfd);
    Touch_Service(&whosonfirst->touch);
    if (!whosonfirst->epaper_initialized) {
        return;
    }

    Epaper_Service(&whosonfirst->epaper.display);

    if (whosonfirst->epaper_clear_pending && Epaper_IsReady(&whosonfirst->epaper.display)) {
        Epaper *const display = &whosonfirst->epaper.display;
        Epaper_Fill(display, 0U, 0U, Epaper_Width(display), Epaper_Height(display),
                    EPAPER_COLOUR_BLACK);
        whosonfirst->epaper_clear_pending = !Epaper_Refresh(display);
    }
}

static void whosonfirst_startup_enter(FSM *fsm) {
    (void) fsm;
    (void) VFD_SetCharacters(&whosonfirst->vfd, "Booting!");
    whosonfirst->startup_message_until_ms = HAL_GetTick() + 3000U;
    whosonfirst->startup_test_card_started = false;
    whosonfirst->epaper_clear_pending = false;
}

static void whosonfirst_startup_service(FSM *fsm) {
    Epaper *const display = &whosonfirst->epaper.display;
    if (whosonfirst->epaper_initialized && !whosonfirst->startup_test_card_started &&
        Epaper_IsReady(display)) {
        Epaper_DrawTestCard(display);
        whosonfirst->startup_test_card_started = Epaper_Refresh(display);
    }

    if ((int32_t) (HAL_GetTick() - whosonfirst->startup_message_until_ms) >= 0) {
        (void) VFD_SetCharacters(&whosonfirst->vfd, "        ");
        FSM_Transition(fsm, MODE_FSM_STATE_IDLE);
    }
}

static void whosonfirst_startup_exit(FSM *fsm) {
    (void) fsm;
    whosonfirst->epaper_clear_pending = whosonfirst->epaper_initialized;
}

static Callbacks whosonfirst_state_callbacks[MODE_FSM_STATE_COUNT] = {
    [MODE_FSM_STATE_INIT] = {
        .enter = whosonfirst_init_enter,
    },
    [MODE_FSM_STATE_STARTUP] = {
        .enter = whosonfirst_startup_enter,
        .service = whosonfirst_startup_service,
        .exit = whosonfirst_startup_exit,
    },
    [MODE_FSM_STATE_IDLE] = {0},
    [MODE_FSM_STATE_ATTRACT] = {0},
    [MODE_FSM_STATE_PREPARE] = {0},
    [MODE_FSM_STATE_READY] = {0},
    [MODE_FSM_STATE_STARTING] = {0},
    [MODE_FSM_STATE_RUNNING] = {0},
    [MODE_FSM_STATE_SOLVED] = {0},
    [MODE_FSM_STATE_ENDED] = {0},
};

Mode_Definition whosonfirst_mode = {
    .state_callbacks = whosonfirst_state_callbacks,
    .always_service = whosonfirst_always_service,
};
