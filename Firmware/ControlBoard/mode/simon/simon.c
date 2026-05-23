#include <xc.h>
#include "simon.h"
#include "../mode_storage.h"
#include "../mode.h"
#include "utils/time.h"
#include <hal/pin.h>
#include <hal/keymatrix.h>

void simon_common_service(void) {
    keymatrix_service(&mode_storage.simon.keymatrix);
}

static void simon_setup_enter(fsm_t *fsm) {
    pin_config(GPIO_SIMON_LED_BLUE, OUTPUT, 0);
    pin_config(GPIO_SIMON_LED_YELLOW, OUTPUT, 0);
    pin_config(GPIO_SIMON_LED_GREEN, OUTPUT, 0);
    pin_config(GPIO_SIMON_LED_RED, OUTPUT, 0);

    pin_write(GPIO_SIMON_LED_BLUE, false);
    pin_write(GPIO_SIMON_LED_YELLOW, false);
    pin_write(GPIO_SIMON_LED_GREEN, false);
    pin_write(GPIO_SIMON_LED_RED, false);

    mode_storage.simon.keymatrix_pins[0] = GPIO_SIMON_BUTTON_BLUE;
    mode_storage.simon.keymatrix_pins[1] = GPIO_SIMON_BUTTON_YELLOW;
    mode_storage.simon.keymatrix_pins[2] = GPIO_SIMON_BUTTON_GREEN;
    mode_storage.simon.keymatrix_pins[3] = GPIO_SIMON_BUTTON_RED;
    mode_storage.simon.keymatrix_pins[4] = PORTPIN_NONE;

    keymatrix_init(&mode_storage.simon.keymatrix, &mode_storage.simon.keymatrix_state[0], &mode_storage.simon.keymatrix_pins[0], NULL, KEYMATRIX_PRESSED_LOW | KEYMATRIX_SENSE_NO_PULL_UPS | KEYMATRIX_DEBOUNCE_20MS | KEYMATRIX_EVENTS_DOWN);

    mode_enable_common_service();
    fsm_transition(fsm, &mode_state_startup);
}

static void simon_startup_enter(fsm_t *fsm) {
    fsm_transition(fsm, &mode_state_idle);
}

static uint8_t i = 0;

static void simon_startup_service(fsm_t *fsm) {

}

const mode_state_func_t simon_funcs[MODE_STATE_COUNT] = {
    { .enter = &simon_setup_enter }, // MODE_STATE_SETUP
    { .enter = &simon_startup_enter, .service = &simon_startup_service }, // MODE_STATE_STARTUP
    {}, // MODE_STATE_IDLE
    {}, // MODE_STATE_ATTRACT
    {}, // MODE_STATE_PREPARE
    {}, // MODE_STATE_READY
    {}, // MODE_STATE_STARTING
    {}, // MODE_STATE_RUNNING
    {}, // MODE_STATE_OVER
    {}, // MODE_STATE_ABORT
};
