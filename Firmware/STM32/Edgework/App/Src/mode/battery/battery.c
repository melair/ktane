#include "mode/battery/battery.h"

#include "mode.h"
#include "slot.h"
#include "sys/gpio.h"

#include <stddef.h>

#define BATTERY_IDENTIFY_PULSE_MS 200U
#define BATTERY_IDENTIFY_GAP_MS   200U
#define BATTERY_IDENTIFY_RESTART_GAP_MS 1000U

static Battery_Data *const battery = &mode_data.mode.battery;

static uint32_t battery_identify_started_at;
static bool battery_was_identifying;

static void battery_set_led(const bool lit) {
    HAL_GPIO_WritePin(GPIO0_Port, GPIO0_Pin, lit ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void battery_service(void) {
    const bool identifying = mode_data.desired_state.identify != 0U;
    if (!identifying) {
        battery_was_identifying = false;
        battery_set_led(true);
        return;
    }

    const uint8_t slot = Slot_Get();
    if (slot == SLOT_UNKNOWN) {
        battery_was_identifying = false;
        battery_set_led(true);
        return;
    }

    if (!battery_was_identifying) {
        battery_identify_started_at = HAL_GetTick();
        battery_was_identifying = true;
    }

    const uint32_t elapsed_ms = HAL_GetTick() - battery_identify_started_at;
    const uint32_t pulse_period_ms = BATTERY_IDENTIFY_PULSE_MS + BATTERY_IDENTIFY_GAP_MS;
    const uint32_t pulses_duration_ms = (uint32_t) slot * pulse_period_ms;
    const uint32_t cycle_duration_ms = pulses_duration_ms + BATTERY_IDENTIFY_RESTART_GAP_MS;
    const uint32_t cycle_elapsed_ms = elapsed_ms % cycle_duration_ms;

    battery_set_led((cycle_elapsed_ms < pulses_duration_ms) &&
                    ((cycle_elapsed_ms % pulse_period_ms) < BATTERY_IDENTIFY_PULSE_MS));
}

static void battery_init_enter(FSM *fsm) {
    HAL_GPIO_WritePin(GPIO0_Port, GPIO0_Pin, GPIO_PIN_RESET);
    const GPIO_InitTypeDef led_gpio_init = {
        .Pin = GPIO0_Pin,
        .Mode = GPIO_MODE_OUTPUT_PP,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_LOW,
    };
    HAL_GPIO_Init(GPIO0_Port, &led_gpio_init);

    const GPIO_InitTypeDef count_gpio_init = {
        .Pin = GPIO1_Pin,
        .Mode = GPIO_MODE_INPUT,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_LOW,
    };
    HAL_GPIO_Init(GPIO1_Port, &count_gpio_init);

    battery->count = HAL_GPIO_ReadPin(GPIO1_Port, GPIO1_Pin) == GPIO_PIN_SET ? 2U : 1U;
    mode_data.current_state.batteries.count = battery->count;
    mode_data.desired_state.batteries.count = battery->count;
    battery_set_led(true);

    battery_mode.always_service = battery_service;
    (void) FSM_Transition(fsm, EDGEWORK_MODE_STATE_STARTUP);
}

static Mode_Callbacks battery_state_callbacks[EDGEWORK_MODE_STATE_COUNT] = {
    [EDGEWORK_MODE_STATE_INIT] = {
        .enter = battery_init_enter,
    },
};

Mode_Definition battery_mode = {
    .state_callbacks = battery_state_callbacks,
    .always_service = NULL,
};
