#include <xc.h>
#include "controller.h"
#include "ui.h"
#include "../../rng.h"
#include "../../lcd.h"
#include "../../argb.h"
#include "../../buzzer.h"
#include "../../can.h"
#include "../../module.h"
#include "../../mode.h"
#include "../../game.h"
#include "../../tick.h"
#include "../../protocol_module.h"
#include "../../peripherals/timer/segment.h"


#define GAME_RNG_MASK 0x89b1a96c

uint8_t last_strikes_current = 0;
uint32_t ready_at = 0;

/* Local function prototypes. */
void controller_service(bool first);
void controller_service_idle(bool first);
void controller_service_setup(bool first);
void controller_service_start(bool first);
void controller_service_running(bool first);
void controller_service_over(bool first);
void controller_update_strikes(void);

/**
 * Initialise any components or state that the controller will require.
 */
void controller_initialise(void) {   
    /* Initialise the LCD. */
    lcd_initialize();
    
    /* Initialise the UI. */
    ui_initialise();

    /* Initialise the seven segment display. */
    segment_initialise();

    /* Register state service handlers with mode. */
    mode_register_callback(GAME_ALWAYS, controller_service, NULL);
    mode_register_callback(GAME_IDLE, controller_service_idle, &tick_20hz);
    mode_register_callback(GAME_SETUP, controller_service_setup, &tick_20hz);
    mode_register_callback(GAME_START, controller_service_start, &tick_20hz);
    mode_register_callback(GAME_RUNNING, controller_service_running, &tick_2khz);
    mode_register_callback(GAME_OVER, controller_service_over, &tick_20hz);

    /* Update the game module state for ourselves to be ready and enabled. */
    this_module->enabled = true;
    this_module->ready = true;
    this_module->solved = true;
    this_module->difficulty = 0;
}

/**
 * Service the controllers behaviour.
 */
void controller_service(bool first) {
    /* Service the LCD panel. */
    lcd_service();
    
    /* Service the seven segment display. */
    segment_service();

    /* Service the UI peripherals. */
    ui_service();
}

/* Maintain if a game request has already been sent. */
bool controller_requested_setup = false;

/**
 * *** TEMPORARY ***
 *
 * Handle idle phase of game, start a game after five seconds.
 *
 * @param first true if first call of the state
 */
void controller_service_idle(bool first) {
//    if (first) {
//        controller_requested_setup = false;
//    }
//
//    if (!controller_requested_setup && tick_value > 5000) {
//        controller_requested_setup = true;
//
//        uint32_t seed = rng_generate(&base_seed, GAME_RNG_MASK);
//
//        /* Create the game. */
//        game_create(seed, 3, 5, 0);
//    }
}

/**
 * *** TEMPORARY ***
 *
 * Handle setup phase of game, enable all modules and wait for them to be ready.
 *
 * Once ready, move to start.
 *
 * @param first true if first call of the state
 */
void controller_service_setup(bool first) {
    if (first) {
        segment_set_colon(false);
        segment_set_digit(0, characters[DIGIT_DASH]);
        segment_set_digit(1, characters[DIGIT_DASH]);
        segment_set_digit(2, characters[DIGIT_DASH]);
        segment_set_digit(3, characters[DIGIT_DASH]);

        this_module->enabled = true;
        this_module->ready = true;

        ready_at = 0;
    }

    bool all_ready = true;
    bool at_least_one_puzzle = false;

    for (uint8_t i = 0; i < MODULE_COUNT; i++) {
        module_game_t *that_module = module_get_game(i);

        if (that_module == NULL) {
            break;
        }

        if (that_module->puzzle) {
            at_least_one_puzzle = true;

            if (!that_module->enabled) {
                game_module_config_send(that_module->id, true, 255);
                all_ready = false;
            } else if (!that_module->ready) {
                all_ready = false;
            }
        }
    }

    if (all_ready && at_least_one_puzzle) {
        if (ready_at == 0) {
            ready_at = tick_value + 3000;
        } else if (ready_at <= tick_value) {
            game_set_state(GAME_START, RESULT_NONE);
        }
    } else {
        ready_at = 0;
    }
}

/* Time start phase started.*/
uint32_t start_time = 0;
/* Second countdown value. */
uint8_t start_countdown = 5;

/**
 * Handle start phase of game, count down from 5 to 1, then move into running.
 * Display seconds until start on segment display, once ready, move to start.
 *
 * @param first true if first call of the state
 */
void controller_service_start(bool first) {
    if (first) {
        controller_update_strikes();
        last_strikes_current = 0;
        start_countdown = 5;
        start_time = tick_value;
    }

    if ((tick_value - start_time) > 1000) {
        start_time = tick_value;
        buzzer_on_timed(BUZZER_DEFAULT_VOLUME, BUZZER_FREQ_A6_SHARP, 40);

        segment_set_colon(false);
        segment_set_digit(0, characters[DIGIT_0 + start_countdown]);
        segment_set_digit(1, characters[DIGIT_0 + start_countdown]);
        segment_set_digit(2, characters[DIGIT_0 + start_countdown]);
        segment_set_digit(3, characters[DIGIT_0 + start_countdown]);

        if (start_countdown == 0) {
            game_set_state(GAME_RUNNING, RESULT_NONE);
        }

        start_countdown--;
    }
}

/**
 * Update the strikes display.
 */
void controller_update_strikes(void) {
    for (uint8_t i = 0; i < game.strikes_total; i++) {
        if (i < game.strikes_current) {
            argb_set(1 + i, 31, 255, 0, 0);
        } else {
            argb_set(1 + i, 31, 0, 255, 0);
        }
    }
}

/**
 * Handle running phase of game, maintain the segment display timer, as well
 * as audible sounds. Monitor strikes for early failure, timeout for failure
 * and all modules being solved for success.
 *
 * @param first true if first call of the state
 */
void controller_service_running(bool first) {
    if (first) {
        game_module_solved(true);
    }

    if (!game.time_remaining.done) {
        uint8_t seconds = game.time_remaining.seconds % 10;
        uint8_t tenseconds = game.time_remaining.seconds / 10;

        if (game.time_remaining.centiseconds == 88) {
            buzzer_on_timed(BUZZER_DEFAULT_VOLUME, BUZZER_FREQ_A6_SHARP, 40);
        }

        if (game.time_ratio == TIME_RATIO_1 && game.time_remaining.centiseconds == 22) {
            buzzer_on_timed(BUZZER_DEFAULT_VOLUME, BUZZER_FREQ_C7_SHARP, 140);
        }

        if (game.time_ratio == TIME_RATIO_1_25 && game.time_remaining.centiseconds == 25) {
            buzzer_on_timed(BUZZER_DEFAULT_VOLUME, BUZZER_FREQ_C7_SHARP, 140);
        }

        /* Send game updates at half way through the second, this should mean
         * any timing corrections (due to drift) are hidden in the count
         * down. */
        if (game.time_remaining.centiseconds == 50) {
            game_update_send();
        }

        if (game.time_remaining.minutes > 0) {
            uint8_t minutes = game.time_remaining.minutes % 10;
            uint8_t tenminutes = game.time_remaining.minutes / 10;

            segment_set_colon(true);

            segment_set_digit(0, characters[DIGIT_0 + tenminutes]);
            segment_set_digit(1, characters[DIGIT_0 + minutes]);

            segment_set_digit(2, characters[DIGIT_0 + tenseconds]);
            segment_set_digit(3, characters[DIGIT_0 + seconds]);
        } else {
            uint8_t centiseconds = game.time_remaining.centiseconds % 10;
            uint8_t tencentiseconds = game.time_remaining.centiseconds / 10;

            segment_set_colon(false);

            segment_set_digit(0, characters[DIGIT_0 + tenseconds]);
            segment_set_digit(1, characters[DIGIT_0 + seconds] | characters[DIGIT_PERIOD]);

            segment_set_digit(2, characters[DIGIT_0 + tencentiseconds]);
            segment_set_digit(3, characters[DIGIT_0 + centiseconds]);
        }
    } else {
        game_set_state(GAME_OVER, RESULT_FAILURE);
    }

    if (game.strikes_current >= game.strikes_total) {
        game_set_state(GAME_OVER, RESULT_FAILURE);
    }

    if (game.strikes_current > last_strikes_current) {
        last_strikes_current = game.strikes_current;
        game.time_ratio = game.strikes_current;
        game_update_send();
        controller_update_strikes();
        buzzer_on_timed(BUZZER_DEFAULT_VOLUME, BUZZER_DEFAULT_FREQUENCY, 750);
    }

    bool game_solved = true;

    for (uint8_t i = 0; i < MODULE_COUNT; i++) {
        module_game_t *that_module = module_get_game(i);

        if (that_module == NULL) {
            break;
        }

        if (!that_module->solved) {
            game_solved = false;
            break;
        }
    }

    if (game_solved) {
        game_set_state(GAME_OVER, RESULT_SUCCESS);
    }
}

/**
 * Handle over phase of game, display end result to user.
 *
 * @param first true if first call of the state
 */
void controller_service_over(bool first){
    if(first) {
        segment_set_colon(false);

        switch(game.result) {
            case RESULT_SUCCESS:
                segment_set_digit(0, characters[DIGIT_S]);
                segment_set_digit(1, characters[DIGIT_A]);
                segment_set_digit(2, characters[DIGIT_F]);
                segment_set_digit(3, characters[DIGIT_E]);
                break;
            case RESULT_FAILURE:
                segment_set_digit(0, characters[DIGIT_D]);
                segment_set_digit(1, characters[DIGIT_E]);
                segment_set_digit(2, characters[DIGIT_A]);
                segment_set_digit(3, characters[DIGIT_D]);
                break;
            case RESULT_NONE:
                segment_set_digit(0, characters[DIGIT_DASH]);
                segment_set_digit(1, characters[DIGIT_DASH]);
                segment_set_digit(2, characters[DIGIT_DASH]);
                segment_set_digit(3, characters[DIGIT_DASH]);
                break;
        }
    }
}