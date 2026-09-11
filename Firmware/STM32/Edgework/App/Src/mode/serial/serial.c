#include "mode/serial/serial.h"

#include "mode.h"

#include <stddef.h>

static Serial_Data *const serial __attribute__((unused)) = &mode_data.mode.serial;

static Mode_Callbacks serial_state_callbacks[EDGEWORK_MODE_STATE_COUNT] = {0};

Mode_Definition serial_mode = {
    .state_callbacks = serial_state_callbacks,
    .always_service = NULL,
};
