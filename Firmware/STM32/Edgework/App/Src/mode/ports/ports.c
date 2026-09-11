#include "mode/ports/ports.h"

#include "mode.h"

#include <stddef.h>

static Ports_Data *const ports __attribute__((unused)) = &mode_data.mode.ports;

static Mode_Callbacks ports_state_callbacks[EDGEWORK_MODE_STATE_COUNT] = {0};

Mode_Definition ports_mode = {
    .state_callbacks = ports_state_callbacks,
    .always_service = NULL,
};
