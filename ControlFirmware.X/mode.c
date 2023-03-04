#include <xc.h>
#include <stdbool.h>
#include <stdint.h>
#include "argb.h"
#include "mode.h"
#include "nvm.h"
#include "tick.h"
#include "game.h"
#include "modes/bootstrap/bootstrap.h"
#include "modes/controller/controller.h"
#include "modes/debug/debug.h"
#include "modes/maze/maze.h"
#include "modes/simon/simon.h"
#include "modes/password/password.h"
#include "modes/whosonfirst/whosonfirst.h"
#include "modes/wires/wires.h"
#include "modes/combination/combination.h"
#include "modes/operator/operator.h"
#include "modes/keys/keys.h"

/* Local function prototypes. */
bool mode_check_if_bootstrap(void);
void mode_unconfigured_state(bool first);
uint8_t mode_find_name_index(uint8_t mode);

/* Mode data. */
mode_data_t mode_data;

/* Modules configured mode*/
uint8_t configured_mode;

/* Function pointers to each stage. */
void (*mode_service_state_function[GAME_STATE_COUNT])(bool);
/* Pointers to tick variable to use to rate limit call. */
bool *mode_service_tick[GAME_STATE_COUNT];
/* Function pointer to service every loop. */
void (*mode_service_always_function)(bool);
/* Pointer to special function receiver. */
void (*mode_special_fn_function)(uint8_t);

/* Last called stage. */
uint8_t last_called_state = 0xff;
/* If the service loop is called for the first time. */
bool service_always_first_call = true;

/* List of node names. */
mode_names_t mode_names[MODE_COUNT+1] = {
    { 0xff, "Unknown" },
    { MODE_BLANK, "Blank" },
    { MODE_BOOTSTRAP, "Bootstrap" },
    { MODE_CONTROLLER, "Controller" },
    { MODE_CONTROLLER_STANDBY, "Standby Controller" },
    { MODE_PUZZLE_DEBUG, "Debug" },
    { MODE_PUZZLE_MAZE, "Maze" },
    { MODE_PUZZLE_SIMON, "Simon Says" },
    { MODE_PUZZLE_PASSWORD, "Password" },
    { MODE_PUZZLE_WHOSONFIRST, "Who's On First" },
    { MODE_PUZZLE_WIRES, "Wires" },
    { MODE_PUZZLE_COMBINATION, "Combination" },
    { MODE_PUZZLE_OPERATOR, "Operator" },
    { MODE_PUZZLE_CARDSCAN, "Card Scan" },
    { MODE_NEEDY_KEYS, "Keys" },
};

/**
 * Find a modes name in the mode_name index.
 *
 * @param mode mode to find
 * @return name index in name array
 */
uint8_t mode_find_name_index(uint8_t mode) {
    for (uint8_t i = 0; i < MODE_COUNT; i++) {
        if (mode_names[i].id == mode) {
            return i;
        }
    }

    return 0;
}

/**
 * Get the mode ID by index.
 * 
 * @param idx index to resolve
 * @return mode ID
 */
uint8_t mode_id_by_index(uint8_t idx) {
    return mode_names[idx].id;
}

/**
 * Get the mode name by index.
 * 
 * @param idx name to resolve
 * @return mode name
 */
uint8_t *mode_name_by_index(uint8_t idx) {
    return mode_names[idx].name;
}

/**
 * Checks to see if the module is in bootstrap mode, this is done by shorting
 * pins A0 and A1 together.
 *
 * @return true if module is in bootstrap
 */
bool mode_check_if_bootstrap(void) {
    /* Set Port A1 (RA1) to be an input. */
    TRISAbits.TRISA1 = 1;

    tick_wait(10);

    /* Check to see if the port is high, if so abort. */
    if (PORTAbits.RA1 == 1) {
        TRISAbits.TRISA1 = 0;
        return false;
    }

    /* Set Port A1 (RA0) to high. */
    LATAbits.LATA0 = 1;

    tick_wait(10);

    /* Check to see if the port now low, if so abort. */
    if (PORTAbits.RA1 == 0) {
        TRISAbits.TRISA1 = 0;
        LATAbits.LATA0 = 0;
        return false;
    }

    /* Set Port A1 (RA0) to low. */
    LATAbits.LATA0 = 0;

    tick_wait(10);

    /* Check to see if the port is high again, if so abort. */
    if (PORTAbits.RA1 == 1) {
        TRISAbits.TRISA1 = 0;
        return false;
    }

    /* Set port back to output. */
    TRISAbits.TRISA1 = 0;

    /* Check passed. */
    return true;
}

/**
 * Get the current mode.
 *
 * @return the current mode
 */
uint8_t mode_get(void) {
    return configured_mode;
}

/**
 * Empty function for unused states in mode.
 *
 * @param first true if first time called
 */
void mode_unconfigured_state(bool first) {
}

/**
 * Run any initialisation functions for the specific mode, an unrecognised mode
 * will be treated as a blank module and error and halt.
 */
void mode_initialise(void) {
    configured_mode = nvm_read(EEPROM_LOC_MODE_CONFIGURATION);

    if (mode_check_if_bootstrap()) {
        configured_mode = MODE_BOOTSTRAP;
    }

    for (uint8_t i = 0; i < GAME_STATE_COUNT; i++) {
        mode_service_state_function[i] = mode_unconfigured_state;
    }

    mode_service_always_function = mode_unconfigured_state;
    mode_special_fn_function = NULL;

    switch(configured_mode) {
        /* Module is completely blank, do nothing. */
        default:
        case MODE_BLANK:
            break;
        /* Module is in bootstrap, A0/A1 shorted. */
        case MODE_BOOTSTRAP:
            bootstrap_initialise();
            break;
        /* Module is a controller. */
        case MODE_CONTROLLER:
            controller_initialise();
            break;
        /* Module is a controller, but in stand by mode. */
        case MODE_CONTROLLER_STANDBY:
            break;
        /* Module is a puzzle, debug. */
        case MODE_PUZZLE_DEBUG:
            debug_initialise();
            break;
        /* Module is a puzzle, maze.*/
        case MODE_PUZZLE_MAZE:
            maze_initialise();
            break;
        /* Module is a puzzle, simon says. */
        case MODE_PUZZLE_SIMON:
            simon_initialise();
            break;
        /* Module is a puzzle, password. */
        case MODE_PUZZLE_PASSWORD:
            password_initialise();
            break;
        /* Module is a puzzle, who's on first.*/
        case MODE_PUZZLE_WHOSONFIRST:
            whosonfirst_initialise();
            break;
        /* Module is a puzzle, wires. */
        case MODE_PUZZLE_WIRES:
            wires_initialise();
            break;
        /* Module is a puzzle, combination locks. */
        case MODE_PUZZLE_COMBINATION:
            combination_initialise();
            break;
        /* Module is a puzzle, operator.*/
        case MODE_PUZZLE_OPERATOR:
            operator_initialise();
            break;
        /* Module is a puzzle, card scan. */
        case MODE_PUZZLE_CARDSCAN:
            cardscan_initialise();
            break;
        /* Module is needy, keys. */
        case MODE_NEEDY_KEYS:
            keys_initialise();
            break;
    }
}

/**
 * Service the mode.
 */
void mode_service(void) {
    mode_service_always_function(service_always_first_call);
    if (service_always_first_call) {
        service_always_first_call = false;
    }

    if (mode_service_tick[game.state] != NULL && *mode_service_tick[game.state] == false) {
        return;
    }

    bool first = last_called_state != game.state;
    last_called_state = game.state;
    mode_service_state_function[game.state](first);
}

/**
 * Register the modes callback against a game state. This prevents duplication
 * of state switching in modules.
 *
 * @param state to call function for
 * @param func function to call
 */
void mode_register_callback(uint8_t state, void (*func)(bool), bool *tick) {
    if (state == 0xff) {
        mode_service_always_function = func;
    } else {
        mode_service_state_function[state] = func;
        mode_service_tick[state] = tick;
    }
}

/**
 * Register the modes callback function for receiving special function messages.
 * 
 * @param func special function call back
 */
void mode_register_special_fn_callback(void (*func)(uint8_t)) {
    mode_special_fn_function = func;   
}

/**
 * Receive special function message, call the callback.
 * 
 * @param special_function special function id called
 */
void mode_call_special_function(uint8_t special_function) {
    if (mode_special_fn_function != NULL) {
        mode_special_fn_function(special_function);
    }
}