#include "mode/indicator/indicator.h"

#include "mode.h"

#include <stddef.h>

static Indicator_Data *const indicator __attribute__((unused)) = &mode_data.mode.indicator;

static Mode_Callbacks indicator_state_callbacks[EDGEWORK_MODE_STATE_COUNT] = {0};

Mode_Definition indicator_mode = {
    .state_callbacks = indicator_state_callbacks,
    .always_service = NULL,
};
