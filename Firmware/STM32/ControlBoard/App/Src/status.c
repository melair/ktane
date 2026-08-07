#include "status.h"
#include "sys/gpio.h"
#include "sys/input_manager.h"
#include "stm32h5xx_hal.h"

static IM_EventQueue button_queue = {0};
static IM_DigitalChannelState button_channel_state[1] = {0};
static IM_DigitalInputState button_input_state = {0};
static IM_DigitalInputConfig button_input_config = {0};
static IM_Handle button_handle = IM_INVALID_HANDLE;

void Status_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Default value to high, LED off */
    HAL_GPIO_WritePin(STATUS_Port, STATUS_Pin, GPIO_PIN_SET);

    /* Configure LED */
    GPIO_InitStruct.Pin = STATUS_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(STATUS_Port, &GPIO_InitStruct);

    button_input_config = (IM_DigitalInputConfig) {
        .rows = &GPIO_Button_Pin,
        .row_count = 1,
        .cols = NULL,
        .col_count = 0,
        .queue = &button_queue,
        .event_mask = IM_EVENT_DOWN | IM_EVENT_HELD | IM_EVENT_UP,
        .state = &button_input_state,
        .scan_period_ms = 10,
        .debounce_ms = 30,
        .held_event_interval_ms = 250,
        .enable_internal_pullups = false,
    };

    button_input_state = (IM_DigitalInputState) {
        .channels = button_channel_state,
    };

    button_handle = IM_RegisterDigital(&button_input_config);
}

void Status_Service(void) {
    IM_Event event;

    while (IM_EventQueue_Read(&button_queue, &event)) {
        if (event.handle != button_handle) {
            continue;
        }

        if (event.event == IM_EVENT_DOWN) {
            HAL_GPIO_WritePin(STATUS_Port, STATUS_Pin, GPIO_PIN_SET);
        } else if (event.event == IM_EVENT_UP) {
            HAL_GPIO_WritePin(STATUS_Port, STATUS_Pin, GPIO_PIN_RESET);
        }
    }
}
