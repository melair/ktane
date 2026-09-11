#include "mode/battery/battery.h"

#include "mode.h"

#include <stddef.h>

static Battery_Data *const battery __attribute__((unused)) = &mode_data.mode.battery;

static Mode_Callbacks battery_state_callbacks[EDGEWORK_MODE_STATE_COUNT] = {0};

Mode_Definition battery_mode = {
    .state_callbacks = battery_state_callbacks,
    .always_service = NULL,
};
