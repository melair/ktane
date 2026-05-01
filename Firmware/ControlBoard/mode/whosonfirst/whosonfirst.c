#include <xc.h>
#include "whosonfirst.h"
#include "../mode.h"

const mode_state_func_t whosonfirst_funcs[MODE_STATE_COUNT] = {
    {}, // MODE_STATE_SETUP
    {}, // MODE_STATE_STARTUP
    {}, // MODE_STATE_IDLE
    {}, // MODE_STATE_ATTRACT
    {}, // MODE_STATE_PREPARE
    {}, // MODE_STATE_READY
    {}, // MODE_STATE_STARTING
    {}, // MODE_STATE_RUNNING
    {}, // MODE_STATE_OVER
    {}, // MODE_STATE_ABORT
};
