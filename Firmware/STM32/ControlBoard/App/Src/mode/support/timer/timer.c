#include "mode/support/timer/timer.h"
#include "mode.h"
#include "module_fsm.h"
#include "sys/gpio.h"
#include "sys/spi.h"

#define TIMER_ROTARY_COUNTS_PER_DETENT 4

static Timer_Data *const timer = &module_data.mode.timer;

static void timer_init_enter(FSM *fsm) {
    timer->strip = (ARGB_Strip){
        .count = TIMER_LED_COUNT,
        .colour_order = COLOUR_ORDER_GRB,
        .max_brightness = 0x7f,
        .leds = timer->leds,
    };

    ARGB_Add_Strip(&timer->strip);

    timer->rotary_config = (IM_RotaryEncoderConfig){
        .slot = IM_ROTARY_SLOT_MODULE_A,
        .queue = &timer->input_queue,
        .event_mask = IM_EVENT_ROTARY_DELTA,
        .state = &timer->rotary_state,
        .invert_direction = false,
        .enable_internal_pullups = true,
        .counts_per_detent = TIMER_ROTARY_COUNTS_PER_DETENT,
    };

    IM_RegisterRotaryEncoder(&timer->rotary_config);

    timer->rotary_button_state = (IM_DigitalInputState){
        .channels = timer->rotary_button_channel_state,
    };

    timer->rotary_button_config = (IM_DigitalInputConfig){
        .rows = &GPIO_A_Pins[5],
        .row_count = 1,
        .queue = &timer->input_queue,
        .event_mask = IM_EVENT_DOWN | IM_EVENT_UP,
        .state = &timer->rotary_button_state,
        .scan_period_ms = 10,
        .debounce_ms = 30,
        .enable_internal_pullups = true,
        .active_high = false,
    };

    timer->rotary_button_handle = IM_RegisterDigital(&timer->rotary_button_config);

    GPIO_InitTypeDef gpio_init = {0};

    HAL_GPIO_WritePin(GPIO_A0_Port, GPIO_A0_Pin, GPIO_PIN_RESET);

    gpio_init.Pin = GPIO_A0_Pin;
    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIO_A0_Port, &gpio_init);

    HAL_GPIO_WritePin(GPIO_A1_Port, GPIO_A1_Pin, GPIO_PIN_RESET);

    gpio_init.Pin = GPIO_A1_Pin;
    HAL_GPIO_Init(GPIO_A1_Port, &gpio_init);

    HAL_GPIO_WritePin(GPIO_A7_Port, GPIO_A7_Pin, GPIO_PIN_RESET);

    gpio_init.Pin = GPIO_A7_Pin;
    gpio_init.Mode = GPIO_MODE_OUTPUT_OD;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIO_A7_Port, &gpio_init);

    timer->strikes_transaction = (SPI_Transaction){
        .bits = 8,
        .baud = SPI_BAUD_125KHZ,
        .operation = SPI_OPERATION_WRITE,
        .cs_port = GPIO_A1_Port,
        .cs_pin = GPIO_A1_Pin,
        .cs_hold = false,
        .lsb_first = false,
        .cke = false,
        .ckp = false,
        .tx_data = &timer->strikes_data,
        .tx_size = sizeof(timer->strikes_data),
    };

    /* Clear the strikes shift register. */
    timer->strikes_data = (STRIKES_DIGIT_BLANK << 16) | STRIKES_DIGIT_BLANK;
    SPI_Queue(&timer->strikes_transaction);

    FSM_Transition(fsm, MODULE_FSM_STATE_STARTUP);
}

static Callbacks timer_state_callbacks[MODULE_FSM_STATE_COUNT] = {
    [MODULE_FSM_STATE_INIT] = {
        .enter = timer_init_enter,
    },
    [MODULE_FSM_STATE_STARTUP] = {0},
    [MODULE_FSM_STATE_IDLE] = {0},
    [MODULE_FSM_STATE_ATTRACT] = {0},
    [MODULE_FSM_STATE_PREPARE] = {0},
    [MODULE_FSM_STATE_READY] = {0},
    [MODULE_FSM_STATE_STARTING] = {0},
    [MODULE_FSM_STATE_RUNNING] = {0},
    [MODULE_FSM_STATE_OVER] = {0},
};

Mode_Definition timer_mode = {
    .state_callbacks = timer_state_callbacks,
    .always_service = NULL,
};
