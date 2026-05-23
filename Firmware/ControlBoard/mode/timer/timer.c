#include <xc.h>
#include <hal/argb.h>
#include "timer.h"
#include "../mode.h"
#include "../mode_storage.h"

/* NOTE: Timer ARGB array is too power hungry to use all colours simultanously at full brightness! */

static void timer_setup_enter(fsm_t *fsm) {
    /* Replace ARGB buffer for timer. */
    argb_set_buffer(&mode_storage.timer.argb_buffer[0], ARGB_DEFAULT_BUFFER_SIZE + TIMER_ARGB_COUNT);

    /* Configure pins for strikes. */
    pin_config(GPIO_TIMER_STRIKES_LATCH, OUTPUT, 0);
    pin_config(GPIO_TIMER_STRIKES_BLANK, OUTPUT, 0);
    pin_write(GPIO_TIMER_STRIKES_LATCH, true);
    pin_write(GPIO_TIMER_STRIKES_BLANK, false);

    fsm_transition(fsm, &mode_state_startup);
}

static void timer_startup_enter(fsm_t *fsm) {
    fsm_transition(fsm, &mode_state_idle);
}

const mode_state_func_t timer_funcs[MODE_STATE_COUNT] = {
    { .enter = &timer_setup_enter }, // MODE_STATE_SETUP
    { .enter = &timer_startup_enter }, // MODE_STATE_STARTUP
    {}, // MODE_STATE_IDLE
    {}, // MODE_STATE_ATTRACT
    {}, // MODE_STATE_PREPARE
    {}, // MODE_STATE_READY
    {}, // MODE_STATE_STARTING
    {}, // MODE_STATE_RUNNING
    {}, // MODE_STATE_OVER
    {}, // MODE_STATE_ABORT
};
