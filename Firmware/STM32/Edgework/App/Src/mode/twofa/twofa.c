#include "mode/twofa/twofa.h"

#include "mode.h"

#include <stddef.h>

static TwoFA_Data *const twofa __attribute__((unused)) = &mode_data.mode.twofa;

static Mode_Callbacks twofa_state_callbacks[EDGEWORK_MODE_STATE_COUNT] = {0};

Mode_Definition twofa_mode = {
    .state_callbacks = twofa_state_callbacks,
    .always_service = NULL,
};
