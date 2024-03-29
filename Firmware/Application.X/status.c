#include <xc.h>
#include <stdbool.h>
#include "argb.h"
#include "status.h"
#include "tick.h"

/* Colours for different status modes. */
const uint8_t led_colors[8][3] = {
    { 0xff, 0x5f, 0x00 },
    { 0xff, 0x00, 0x00 },
    { 0x00, 0xff, 0x00 },
    { 0xff, 0x00, 0x00 },
    { 0x00, 0xff, 0x00 },
    { 0x00, 0x00, 0x00 }
};

/* Current status. */
uint8_t current_status = STATUS_IDLE;
/* Current error. */
uint8_t current_error = ERROR_NONE;
/* Are we identifying. */
bool identify = false;

/* Phase in error animation. */
uint8_t phase = 0;
/* Next tick to update error animation. */
uint8_t wait_ticks = 0;

/**
 * Set the status indicator to the new mode.
 *
 * @param status new status
 */
void status_set(uint8_t status) {
    if (current_status == status) {
        return;
    }

    current_status = status;
    phase = 0;
}

/**
 * Set the status indicator to an animated error mode.
 *
 * @param error error mode
 */
void status_error(uint8_t error) {
    if (current_error == error) {
        return;
    }

    current_error = error;
    phase = 0;
}

/**
 * Set status indicator to identify.
 *
 * @param on true if in identify mode
 */
void status_identify(bool on) {
    identify = on;
}

/**
 * Service the status indicator and animate error if needed.
 */
void status_service(void) {
    if (!tick_20hz) {
        return;
    }

    wait_ticks++;
    if (wait_ticks < 4) {
        return;
    }

    wait_ticks = 0;

    if (current_error == ERROR_NONE) {
        switch(phase) {
            case 0:
                argb_set_status(led_colors[current_status][0], led_colors[current_status][1], led_colors[current_status][2]);
                break;
            case 1:
                if (current_status == 1 || current_status == 2) {
                    argb_set_status(0, 0, 0);
                }
            case 2:
                if (identify == true) {
                    argb_set_status(0, 0, 255);
                }
                break;
        }

        phase++;
        if (phase > 2) {
            phase = 0;
        }
    } else {
        switch(phase) {
            case 0:
                argb_set_status(led_colors[current_status][0], led_colors[current_status][1], led_colors[current_status][2]);
                break;
            case 1:
                argb_set_status(0, 0, 0);
                break;
            case 2:
                argb_set_status(255, 0, 0);
                break;
            case 3:
                argb_set_status(0, 0, 0);
                break;
            case 4:
                argb_set_status(255, 0, 0);
                break;
            case 5:
                argb_set_status(0, 0, 0);
                break;
            case 6:
                if (current_error == ERROR_LOCAL) {
                    argb_set_status(0, 255, 0);
                } else if (current_error == ERROR_REMOTE_ACTIVE) {
                    argb_set_status(0, 0, 255);
                } else {
                    argb_set_status(255, 0, 255);
                }
                break;
            case 7:
                argb_set_status(0, 0, 0);
                break;
            case 8:
                if (identify == true) {
                    argb_set_status(0, 0, 255);
                }
                break;
            case 12:
                argb_set_status(0, 0, 0);
                break;
        }

        phase++;
        if (phase == 13) {
            phase = 0;
        }
    }
}
