#include "mode/controller/controller.h"

#include "main.h"
#include "mode.h"
#include "sys/gpio.h"

#include <stddef.h>

#define CONTROLLER_POWER_SCAN_PERIOD_MS 10U
#define CONTROLLER_POWER_DEBOUNCE_MS     30U

static Controller_Data *const controller = &mode_data.mode.controller;
static const GPIO_PinDef controller_power_input_pin = {GPIO4_Port, GPIO4_Pin};

static void controller_set_powered(const bool powered) {
    HAL_GPIO_WritePin(GPIO11_Port, GPIO11_Pin, powered ? GPIO_PIN_SET : GPIO_PIN_RESET);
    mode_data.current_state.controller.flags.powered = powered ? 1U : 0U;
}

static void controller_service(void) {
    const bool powered = IM_DigitalChannel_Get(&controller->power_input_config, 0U, IM_NO_COLUMN);

    controller_set_powered(powered);
}

static void controller_init_enter(FSM *fsm) {
    HAL_GPIO_WritePin(GPIO11_Port, GPIO11_Pin, GPIO_PIN_RESET);
    const GPIO_InitTypeDef output_gpio_init = {
        .Pin = GPIO11_Pin,
        .Mode = GPIO_MODE_OUTPUT_PP,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_LOW,
    };
    HAL_GPIO_Init(GPIO11_Port, &output_gpio_init);

    controller->power_input_state = (IM_DigitalInputState) {
        .channels = controller->power_channel_state,
    };
    controller->power_input_config = (IM_DigitalInputConfig) {
        .rows = &controller_power_input_pin,
        .row_count = 1U,
        .state = &controller->power_input_state,
        .scan_period_ms = CONTROLLER_POWER_SCAN_PERIOD_MS,
        .debounce_ms = CONTROLLER_POWER_DEBOUNCE_MS,
        .pull = IM_DIGITAL_INPUT_PULL_DOWN,
        .active_high = true,
    };
    controller->power_input_handle = IM_RegisterDigital(&controller->power_input_config);
    if (controller->power_input_handle == IM_INVALID_HANDLE) {
        Error_Handler();
        return;
    }

    controller_set_powered(false);
    controller_mode.always_service = controller_service;
    (void) FSM_Transition(fsm, EDGEWORK_MODE_STATE_STARTUP);
}

static Mode_Callbacks controller_state_callbacks[EDGEWORK_MODE_STATE_COUNT] = {
    [EDGEWORK_MODE_STATE_INIT] = {
        .enter = controller_init_enter,
    },
};

Mode_Definition controller_mode = {
    .state_callbacks = controller_state_callbacks,
    .always_service = NULL,
};
