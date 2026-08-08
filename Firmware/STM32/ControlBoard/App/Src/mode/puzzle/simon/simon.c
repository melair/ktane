#include "mode/puzzle/simon/simon.h"
#include "module_fsm.h"
#include <stddef.h>

static Callbacks simon_state_callbacks[MODULE_FSM_STATE_COUNT] = {
    [MODULE_FSM_STATE_INIT] = {0},
    [MODULE_FSM_STATE_STARTUP] = {0},
    [MODULE_FSM_STATE_IDLE] = {0},
    [MODULE_FSM_STATE_ATTRACT] = {0},
    [MODULE_FSM_STATE_PREPARE] = {0},
    [MODULE_FSM_STATE_READY] = {0},
    [MODULE_FSM_STATE_STARTING] = {0},
    [MODULE_FSM_STATE_RUNNING] = {0},
    [MODULE_FSM_STATE_OVER] = {0},
};

Mode_Definition simon_mode = {
    .state_callbacks = simon_state_callbacks,
    .service = NULL,
};
