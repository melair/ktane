#include "mode/controller/controller.h"

#include "mode.h"

#include <stddef.h>

static Controller_Data *const controller __attribute__((unused)) = &mode_data.mode.controller;

static Mode_Callbacks controller_state_callbacks[EDGEWORK_MODE_STATE_COUNT] = {0};

Mode_Definition controller_mode = {
    .state_callbacks = controller_state_callbacks,
    .always_service = NULL,
};
