#include "mode/puzzle/simon/simon.h"
#include "mode.h"
#include "mode_fsm.h"
#include <stddef.h>

static Simon_Data *const simon = &mode_data.mode.simon;

static Callbacks simon_state_callbacks[MODE_FSM_STATE_COUNT] = {
    [MODE_FSM_STATE_INIT] = {0},
    [MODE_FSM_STATE_STARTUP] = {0},
    [MODE_FSM_STATE_IDLE] = {0},
    [MODE_FSM_STATE_ATTRACT] = {0},
    [MODE_FSM_STATE_PREPARE] = {0},
    [MODE_FSM_STATE_READY] = {0},
    [MODE_FSM_STATE_STARTING] = {0},
    [MODE_FSM_STATE_RUNNING] = {0},
    [MODE_FSM_STATE_OVER] = {0},
};

Mode_Definition simon_mode = {
    .state_callbacks = simon_state_callbacks,
    .always_service = NULL,
};
