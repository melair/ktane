#include <xc.h>
#include "rfid.h"
#include "../mode.h"

static void rfid_setup_enter(fsm_t *fsm) {
    fsm_transition(fsm, &mode_state_startup);
}

static void rfid_startup_enter(fsm_t *fsm) {
    fsm_transition(fsm, &mode_state_idle);
}

const mode_state_func_t rfid_funcs[MODE_STATE_COUNT] = {
    { .enter = &rfid_setup_enter }, // MODE_STATE_SETUP
    { .enter = &rfid_startup_enter }, // MODE_STATE_STARTUP
    {}, // MODE_STATE_IDLE
    {}, // MODE_STATE_ATTRACT
    {}, // MODE_STATE_PREPARE
    {}, // MODE_STATE_READY
    {}, // MODE_STATE_STARTING
    {}, // MODE_STATE_RUNNING
    {}, // MODE_STATE_OVER
    {}, // MODE_STATE_ABORT
};
