#include <xc.h>
#include <stdbool.h>
#include "combination.h"
#include "../../peripherals/segment.h"
#include "../../hal/pins.h"
#include "../../peripherals/rotary.h"
#include "../../peripherals/keymatrix.h"
#include "../../edgework.h"
#include "../../mode.h"
#include "../../module.h"
#include "../../game.h"
#include "../../argb.h"
#include "../../buzzer.h"
#include "../../rng.h"
#include "../../tick.h"
#include "../../sound.h"

/* Keymatrix. */
pin_t combination_cols[] = {KPIN_B2, KPIN_NONE};
pin_t combination_rows[] = {KPIN_NONE};

void combination_service(bool first);
void combination_service_idle(bool first);
void combination_service_running(bool first);
void combination_service_setup(bool first);
void combination_check(void);
void combination_setup(void);
void combination_disable(bool first);

#define COMBINATION_MAX_VALUE 20

void combination_initialise(void) {
    /* Initialise the seven segment display and encoder. */
    segment_initialise();
    rotary_initialise(KPIN_B1, KPIN_B0);

    /* Initialise keymatrix. */
    keymatrix_initialise(&combination_cols[0], &combination_rows[0], KEYMODE_COL_ONLY);

    /* Register state service handlers with mode. */
    mode_register_callback(GAME_ALWAYS, combination_service, NULL);
    mode_register_callback(GAME_IDLE, combination_service_idle, &tick_20hz);
    mode_register_callback(GAME_RUNNING, combination_service_running, &tick_20hz);
    mode_register_callback(GAME_SETUP, combination_service_setup, &tick_20hz);
    mode_register_callback(GAME_DISABLE, combination_disable, NULL);
}

void combination_disable(bool first) {
    for (uint8_t i = 0; i < 4; i++) {
        segment_set_digit(i, characters[DIGIT_SPACE]);
    }
}

void combination_service(bool first) {
    segment_service();
    rotary_service();
    keymatrix_service();
}

void combination_service_idle(bool first) {
    if (first) {
        combination_disable(first);
    }
}

void combination_service_setup(bool first) {
    if (first) {
        keymatrix_clear();
        combination_setup();
        game_module_ready(true);
    }
}

void combination_setup(void) {
    mode_data.combination.stage = 0;
    mode_data.combination.value = ((uint8_t) (game.module_seed & 0xff)) % COMBINATION_MAX_VALUE;
    mode_data.combination.even_clockwise = ((mode_data.combination.value / 10) + (mode_data.combination.value % 10)) % 3 == 0;
    mode_data.combination.clockwise_traveled = false;
    mode_data.combination.anticlockwise_traveled = false;
}

void combination_service_running(bool first) {
    if (this_module->solved) {
        return;
    }

    int8_t delta = rotary_fetch_delta();

    if (delta != 0) {
        if (delta > 0) {
            mode_data.combination.clockwise_traveled = true;
        } else {
            mode_data.combination.anticlockwise_traveled = true;
        }

        sound_play(SOUND_ALL_ROTARY_CLICK);
    }

    mode_data.combination.value += delta;

    if (mode_data.combination.value < 0) {
        mode_data.combination.value = COMBINATION_MAX_VALUE - 1;
    } else if (mode_data.combination.value >= COMBINATION_MAX_VALUE) {
        mode_data.combination.value = 0;
    }

    rotary_clear();

    for (uint8_t press = keymatrix_fetch(); press != KEY_NO_PRESS; press = keymatrix_fetch()) {
        if (press & KEY_DOWN_BIT) {
            sound_play(SOUND_ALL_PRESS_IN);
            combination_check();
        }
    }

    /* Update segment display. */
    uint8_t edge_character = 0;

    switch (mode_data.combination.stage) {
        case 0:
            edge_character = DIGIT_THREESCORE;
            break;
        case 1:
            edge_character = DIGIT_EQUALS;
            break;
        case 2:
            edge_character = DIGIT_UNDERSCORE;
            break;
    }

    if (mode_data.combination.stage == 3) {
        segment_set_digit(1, characters[0]);
        segment_set_digit(2, characters[0]);
    } else {
        uint8_t digit = ((uint8_t) mode_data.combination.value / 10);
        segment_set_digit(1, characters[1 + digit]);
        digit = mode_data.combination.value % 10;
        segment_set_digit(2, characters[1 + digit]);
    }

    segment_set_digit(0, characters[edge_character]);
    segment_set_digit(3, characters[edge_character]);
}

void combination_check(void) {
    if (edgework_twofa_present()) {
        mode_data.combination.expected[0] = edgework_twofa_digit(0) + edgework_twofa_digit(1);
        mode_data.combination.expected[1] = edgework_twofa_digit(4) + edgework_twofa_digit(5);
    } else {
        uint8_t module_count = module_get_count_enabled_module();
        uint8_t solved_count = module_get_count_enabled_solved_puzzle();
        mode_data.combination.expected[0] = edgework_serial_last_digit() + solved_count;
        mode_data.combination.expected[1] = module_count;
    }

    mode_data.combination.expected[2] = (mode_data.combination.entered[0] + mode_data.combination.entered[1]) % COMBINATION_MAX_VALUE;

    bool correct = false;

    /* Skip and strike if both directions, or if neither. */
    if (mode_data.combination.clockwise_traveled || mode_data.combination.anticlockwise_traveled) {
        bool shouldBeClockwise = (mode_data.combination.stage % 2) == (mode_data.combination.even_clockwise ? 0 : 1);

        /* Skip if wrong direction, stage is 0 indexed. */
        if ((shouldBeClockwise && mode_data.combination.clockwise_traveled) || (!shouldBeClockwise && mode_data.combination.anticlockwise_traveled)) {
            /* If we have the correct value, great. */
            if (mode_data.combination.expected[mode_data.combination.stage] == mode_data.combination.value) {
                mode_data.combination.entered[mode_data.combination.stage] = (uint8_t) mode_data.combination.value;
                correct = true;
            }
        }
    }

    if (correct) {
        mode_data.combination.stage++;
    } else {
        game_module_strike(1);
        mode_data.combination.stage = 0;
        mode_data.combination.value = ((uint8_t) (game.module_seed & 0xff)) % COMBINATION_MAX_VALUE;
        mode_data.combination.even_clockwise = ((mode_data.combination.value / 10) + (mode_data.combination.value % 10)) % 3 == 0;
    }

    mode_data.combination.clockwise_traveled = false;
    mode_data.combination.anticlockwise_traveled = false;

    if (mode_data.combination.stage == 3) {
        game_module_solved(true);
    }
}