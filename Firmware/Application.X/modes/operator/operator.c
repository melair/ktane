#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include "operator.h"
#include "../../argb.h"
#include "../../edgework.h"
#include "../../game.h"
#include "../../mode.h"
#include "../../hal/pins.h"
#include "../../peripherals/keymatrix.h"
#include "../../rng.h"
#include "../../tick.h"
#include "../../sound.h"

/* Rotary dial as keymatrix. */
pin_t operator_cols[] = {KPIN_A0, KPIN_A1, KPIN_A2, KPIN_NONE};
pin_t operator_rows[] = {KPIN_NONE};

void operator_service(bool first);
void operator_service_idle(bool first);
void operator_service_running(bool first);
void operator_service_setup(bool first);
void operator_disable(bool first);

#define OPERATOR_RNG_MASK 0xd7e083da

#define OPERATOR_CODE_COUNT 54

const uint8_t operator_area[OPERATOR_CODE_COUNT][5] = {
    { 0, 1, 7, 4, 7},
    { 0, 1, 8, 6, 9},
    { 0, 1, 9, 2, 2},
    { 0, 1, 4, 8, 5},
    { 0, 1, 2, 4, 4},
    { 0, 1, 2, 4, 3},
    { 0, 1, 2, 8, 5},
    { 0, 1, 2, 0, 6},
    { 0, 1, 6, 0, 6},
    { 0, 1, 4, 5, 5},
    { 0, 1, 3, 0, 2},
    { 0, 1, 3, 0, 5},
    { 0, 1, 9, 0, 5},
    { 0, 1, 6, 8, 4},
    { 0, 2, 0, 0xff, 0xff},
    { 0, 1, 7, 2, 8},
    { 0, 1, 4, 9, 2},
    { 0, 1, 4, 5, 2},
    { 0, 1, 4, 8, 0},
    { 0, 2, 0, 0xff, 0xff},
    { 0, 2, 0, 0xff, 0xff},
    { 0, 1, 2, 5, 5},
    { 0, 1, 2, 0, 4},
    { 0, 1, 4, 7, 3},
    { 0, 1, 5, 2, 4},
    { 0, 1, 6, 6, 1},
    { 0, 1, 6, 0, 6},
    { 0, 1, 1, 6, 0xff},
    { 0, 1, 6, 1, 0xff},
    { 0, 1, 6, 0, 6},
    { 0, 1, 6, 0, 6},
    { 0, 1, 6, 0, 6},
    { 0, 1, 6, 0, 3},
    { 0, 1, 6, 1, 0xff},
    { 0, 1, 2, 5, 4},
    { 0, 1, 8, 8, 9},
    { 0, 1, 6, 3, 4},
    { 0, 1, 2, 7, 0},
    { 0, 1, 3, 0, 4},
    { 0, 1, 4, 7, 7},
    { 0, 1, 9, 3, 7},
    { 0, 1, 3, 2, 7},
    { 0, 1, 2, 1, 0xff},
    { 0, 1, 9, 6, 2},
    { 0, 1, 9, 0, 5}
};

void operator_initialise(void) {
    /* Initialise ARGB expanded memory. */
    argb_expand(OPERATOR_ARGB_COUNT, &mode_data.operator.argb_leds[0], &mode_data.operator.argb_output[0]);

    /* Register state service handlers with mode. */
    mode_register_callback(GAME_ALWAYS, operator_service, NULL);
    mode_register_callback(GAME_IDLE, operator_service_idle, &tick_20hz);
    mode_register_callback(GAME_RUNNING, operator_service_running, &tick_20hz);
    mode_register_callback(GAME_SETUP, operator_service_setup, &tick_20hz);
    mode_register_callback(GAME_DISABLE, operator_disable, NULL);

    /* Initialise keymatrix. */
    keymatrix_initialise(&operator_cols[0], &operator_rows[0], KEYMODE_COL_ONLY);

    /* Rotary dials should have a 66.6ms break period, and 33.3ms make period
     * after. Lowering the required number of sample periods to detect a change
     * to 1 seems wrong, but it would unlikely that we would detect an initial
     * bounce and for it to still be bouncing 10ms later. */
    keymatrix_required_periods(1);
}

void operator_service(bool first) {
    keymatrix_service();
}

void operator_disable(bool first) {
    for (uint8_t i = 0; i < OPERATOR_ARGB_COUNT; i++) {
        argb_set_module(i, 0, 0, 0);
    }
}

void operator_service_idle(bool first) {
    if (first) {
        operator_disable(first);
    }
}

void operator_service_setup(bool first) {
    if (first) {
        mode_data.operator.sound_playback = 0xff;

        uint8_t area_code = rng_generate8(&game.module_seed, OPERATOR_RNG_MASK) % OPERATOR_CODE_COUNT;
        mode_data.operator.sound_numbers[0] = area_code;
        mode_data.operator.sound_pos = 1;

        uint8_t area_code_length = 0;

        for (uint8_t i = 0; i < 5; i++) {
            if (operator_area[area_code][i] != 0xff) {
                mode_data.operator.wanted_numbers[i] = operator_area[area_code][i];
                area_code_length++;
            }
        }

        for (uint8_t i = 0; i < 5; i++) {
            uint8_t num = rng_generate8(&game.module_seed, OPERATOR_RNG_MASK) % 10;
            mode_data.operator.wanted_numbers[area_code_length] = num;
            mode_data.operator.sound_numbers[i + 1] = num;
            area_code_length++;
            mode_data.operator.sound_pos++;
        }

        mode_data.operator.wanted_pos = area_code_length;

        game_module_ready(true);
    }
}

void operator_service_running(bool first) {
    if (first) {
        keymatrix_clear();
        mode_data.operator.dialed_pos = 0;
    }

    if (this_module->solved) {
        return;
    }

    /* Handle rotary changes. */
    for (uint8_t press = keymatrix_fetch(); press != KEY_NO_PRESS; press = keymatrix_fetch()) {
        /* Down or up? */
        bool down = (press & KEY_DOWN_BIT);

        /* Map bits from keymatrix into a number. */
        uint8_t key = (press & KEY_COL_BITS);

        if (key == 0) {
            /* Key matrix works on assumption that pins are down when they are
             * shorted to GND, however the rotary dial breaks the GND connection
             * when a pulse is sent. So down is actually the release here. */
            if (!down && mode_data.operator.rotary_dialing) {
                mode_data.operator.rotary_pulses++;
            }
        } else if (key == 1) {
            /* The dialing connection works in line with Keymatrix, so down is
             * the rotary dial being activated. */
            if (down) {
                mode_data.operator.rotary_pulses = 0;
                mode_data.operator.rotary_dialing = true;
            } else {
                mode_data.operator.rotary_dialing = false;

                /* Store last dialed number. */
                mode_data.operator.dialed_numbers[mode_data.operator.dialed_pos] = mode_data.operator.rotary_pulses;
                mode_data.operator.dialed_pos++;

                /* Check to see if we're at the size. */
                if (mode_data.operator.dialed_pos == mode_data.operator.wanted_pos) {
                    bool correct = true;

                    for (uint8_t i = 0; i < mode_data.operator.dialed_pos; i++) {
                        if (mode_data.operator.wanted_numbers[i] != mode_data.operator.dialed_numbers[i]) {
                            correct = false;
                        }
                    }

                    mode_data.operator.dialed_pos = 0;

                    if (correct) {
                        game_module_solved(true);
                    } else {
                        game_module_strike(1);
                    }
                }
            }
        } else if (key == 2) {
            if (mode_data.operator.sound_playback == 0xff) {
                mode_data.operator.sound_playback = 0;
                mode_data.operator.sound_tick = 0;
            }
        }
    }

    if (tick_2hz) {
        if (mode_data.operator.sound_playback != 0xff) {
            mode_data.operator.sound_tick++;

            if (mode_data.operator.sound_tick == 0) {
                uint8_t off = (mode_data.operator.sound_playback == 0 ? 10 : 0);
                sound_play(SOUND_OPERATOR_BASE + mode_data.operator.sound_numbers[mode_data.operator.sound_playback] + off);
            }

            if (mode_data.operator.sound_playback == 0) {
                if (mode_data.operator.sound_tick == 3) {
                    mode_data.operator.sound_tick = 0;
                    mode_data.operator.sound_playback++;
                }
            } else {
                if (mode_data.operator.sound_tick == 1) {
                    mode_data.operator.sound_tick = 0;
                    mode_data.operator.sound_playback++;
                }
            }

            if (mode_data.operator.sound_playback == mode_data.operator.sound_pos) {
                mode_data.operator.sound_playback = 0xff;
            }
        }
    }
}
