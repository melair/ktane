#include "mode/puzzle/simon/simon.h"
#include "mode.h"
#include "mode_fsm.h"
#include "pwm/pwm.h"
#include "sys/gpio.h"
#include "stm32h5xx_it.h"
#include <stddef.h>

#define SIMON_BLUE_BUTTON_Pin GPIO_C4_Pin
#define SIMON_BLUE_BUTTON_Port GPIO_C4_Port
#define SIMON_YELLOW_BUTTON_Pin GPIO_C5_Pin
#define SIMON_YELLOW_BUTTON_Port GPIO_C5_Port
#define SIMON_GREEN_BUTTON_Pin GPIO_C6_Pin
#define SIMON_GREEN_BUTTON_Port GPIO_C6_Port
#define SIMON_RED_BUTTON_Pin GPIO_C7_Pin
#define SIMON_RED_BUTTON_Port GPIO_C7_Port

#define SIMON_BLUE_LAMP_Pin GPIO_C0_Pin
#define SIMON_BLUE_LAMP_Port GPIO_C0_Port
#define SIMON_YELLOW_LAMP_Pin GPIO_C1_Pin
#define SIMON_YELLOW_LAMP_Port GPIO_C1_Port
#define SIMON_GREEN_LAMP_Pin GPIO_C2_Pin
#define SIMON_GREEN_LAMP_Port GPIO_C2_Port
#define SIMON_RED_LAMP_Pin GPIO_C3_Pin
#define SIMON_RED_LAMP_Port GPIO_C3_Port

#define SIMON_STARTUP_PULSE_MS 500U
#define SIMON_STARTUP_GAP_MS 100U
#define SIMON_STARTUP_LOOP_COUNT 3U
#define SIMON_LAMP_ON_DUTY_PERCENT 100U
#define SIMON_ATTRACT_LAMP_DURATION_MS 1000U

static Simon_Data *const simon = &mode_data.mode.simon;
static const GPIO_PinDef simon_button_pins[] = {
    {SIMON_BLUE_BUTTON_Port, SIMON_BLUE_BUTTON_Pin},
    {SIMON_YELLOW_BUTTON_Port, SIMON_YELLOW_BUTTON_Pin},
    {SIMON_GREEN_BUTTON_Port, SIMON_GREEN_BUTTON_Pin},
    {SIMON_RED_BUTTON_Port, SIMON_RED_BUTTON_Pin},
};
static const GPIO_PinDef simon_lamp_pins[] = {
    {SIMON_BLUE_LAMP_Port, SIMON_BLUE_LAMP_Pin},
    {SIMON_YELLOW_LAMP_Port, SIMON_YELLOW_LAMP_Pin},
    {SIMON_GREEN_LAMP_Port, SIMON_GREEN_LAMP_Pin},
    {SIMON_RED_LAMP_Port, SIMON_RED_LAMP_Pin},
};

static void simon_lamp_set(const uint8_t lamp_index, const bool enabled) {
    PWM_SetDuty(&simon_lamp_pins[lamp_index], enabled ? SIMON_LAMP_ON_DUTY_PERCENT : 0U);
}

static void simon_lamps_off(void) {
    for (uint8_t lamp_index = 0U;
         lamp_index < sizeof(simon_lamp_pins) / sizeof(simon_lamp_pins[0]);
         lamp_index++) {
        simon_lamp_set(lamp_index, false);
    }
}

static void simon_init_enter(FSM *fsm) {
    for (uint8_t lamp_index = 0U;
         lamp_index < sizeof(simon_lamp_pins) / sizeof(simon_lamp_pins[0]);
         lamp_index++) {
        if (!PWM_Setup(&simon_lamp_pins[lamp_index])) {
            Error_Handler();
        }
    }
    simon_lamps_off();

    simon->button_queue = (IM_EventQueue){0};
    simon->button_input_state = (IM_DigitalInputState){
        .channels = simon->button_channel_state,
    };
    simon->button_input_config = (IM_DigitalInputConfig){
        .rows = simon_button_pins,
        .row_count = sizeof(simon_button_pins) / sizeof(simon_button_pins[0]),
        .queue = &simon->button_queue,
        .event_mask = IM_EVENT_DOWN,
        .state = &simon->button_input_state,
        .scan_period_ms = 10U,
        .debounce_ms = 30U,
        .pull = IM_DIGITAL_INPUT_PULL_UP,
        .active_high = false,
    };
    simon->button_handle = IM_RegisterDigital(&simon->button_input_config);

    FSM_Transition(fsm, MODE_FSM_STATE_STARTUP);
}

static void simon_startup_enter(FSM *fsm) {
    (void) fsm;
    simon_lamps_off();
    simon->startup_lamp_index = 0U;
    simon->startup_loop_count = 0U;
    simon->startup_lamp_on = true;
    simon_lamp_set(simon->startup_lamp_index, true);
    simon->startup_next_change_ms = HAL_GetTick() + SIMON_STARTUP_PULSE_MS;
}

static void simon_startup_service(FSM *fsm) {
    const uint32_t now_ms = HAL_GetTick();
    if ((int32_t) (now_ms - simon->startup_next_change_ms) < 0) {
        return;
    }

    if (simon->startup_lamp_on) {
        simon_lamp_set(simon->startup_lamp_index, false);
        if (simon->startup_lamp_index + 1U ==
            sizeof(simon_lamp_pins) / sizeof(simon_lamp_pins[0])) {
            simon->startup_loop_count++;
            if (simon->startup_loop_count == SIMON_STARTUP_LOOP_COUNT) {
                FSM_Transition(fsm, MODE_FSM_STATE_IDLE);
                return;
            }
            simon->startup_lamp_index = 0U;
        } else {
            simon->startup_lamp_index++;
        }
        simon->startup_lamp_on = false;
        simon->startup_next_change_ms = now_ms + SIMON_STARTUP_GAP_MS;
        return;
    }

    simon->startup_lamp_on = true;
    simon_lamp_set(simon->startup_lamp_index, true);
    simon->startup_next_change_ms = now_ms + SIMON_STARTUP_PULSE_MS;
}

static void simon_idle_enter(FSM *fsm) {
    FSM_Transition(fsm, MODE_FSM_STATE_ATTRACT);
}

static void simon_attract_enter(FSM *fsm) {
    (void) fsm;
    simon_lamps_off();
    IM_EventQueue_Clear(&simon->button_queue);
    for (uint8_t lamp_index = 0U;
         lamp_index < sizeof(simon_lamp_pins) / sizeof(simon_lamp_pins[0]);
         lamp_index++) {
        simon->attract_lamp_on[lamp_index] = false;
        simon->attract_lamp_until_ms[lamp_index] = 0U;
    }
}

static void simon_attract_service(FSM *fsm) {
    (void) fsm;
    const uint32_t now_ms = HAL_GetTick();
    IM_Event event;
    while (IM_EventQueue_Read(&simon->button_queue, &event)) {
        if (event.event != IM_EVENT_DOWN ||
            event.channel >= sizeof(simon_lamp_pins) / sizeof(simon_lamp_pins[0])) {
            continue;
        }
        simon_lamp_set(event.channel, true);
        simon->attract_lamp_on[event.channel] = true;
        simon->attract_lamp_until_ms[event.channel] = now_ms + SIMON_ATTRACT_LAMP_DURATION_MS;
    }

    for (uint8_t lamp_index = 0U;
         lamp_index < sizeof(simon_lamp_pins) / sizeof(simon_lamp_pins[0]);
         lamp_index++) {
        if (simon->attract_lamp_on[lamp_index] &&
            (int32_t) (now_ms - simon->attract_lamp_until_ms[lamp_index]) >= 0) {
            simon_lamp_set(lamp_index, false);
            simon->attract_lamp_on[lamp_index] = false;
        }
    }
}

static Callbacks simon_state_callbacks[MODE_FSM_STATE_COUNT] = {
    [MODE_FSM_STATE_INIT] = {
        .enter = simon_init_enter,
    },
    [MODE_FSM_STATE_STARTUP] = {
        .enter = simon_startup_enter,
        .service = simon_startup_service,
    },
    [MODE_FSM_STATE_IDLE] = {
        .enter = simon_idle_enter,
    },
    [MODE_FSM_STATE_ATTRACT] = {
        .enter = simon_attract_enter,
        .service = simon_attract_service,
    },
    [MODE_FSM_STATE_PREPARE] = {0},
    [MODE_FSM_STATE_READY] = {0},
    [MODE_FSM_STATE_STARTING] = {0},
    [MODE_FSM_STATE_RUNNING] = {0},
    [MODE_FSM_STATE_SOLVED] = {0},
    [MODE_FSM_STATE_ENDED] = {0},
};

Mode_Definition simon_mode = {
    .state_callbacks = simon_state_callbacks,
    .always_service = NULL,
};
