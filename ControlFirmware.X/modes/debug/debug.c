#include <xc.h>
#include <stdbool.h>
#include "debug.h"
#include "../../mode.h"
#include "../../game.h"
#include "../../tick.h"
#include "../../peripherals/ports.h"
#include "../../peripherals/keymatrix.h"

/* Local function prototypes. */
void debug_game_setup(bool first);
void debug_game_running(bool first);

/* Initialise keymatrix. */
pin_t debug_cols[] = {KPIN_A5, KPIN_A6, KPIN_A7, KPIN_NONE};
pin_t debug_rows[] = {KPIN_NONE};

/**
 * Initialise the debug puzzle, a simple puzzle that allows declaration of ready,
 * make a strike or mark solved.
 */
void debug_initialise(void) {
    /* Make debug pins input. */
    KTRISA |= 0b11100000;
    
    /* Enable weak pull up, ground to activate. */
    KWPUA |= 0b11100000;
    
    /* Register our callbacks. */
    mode_register_callback(GAME_ALWAYS, debug_service);
    mode_register_callback(GAME_SETUP, debug_game_setup);
    mode_register_callback(GAME_RUNNING, debug_game_running);
    
    /* Initialise keymatrix. */
    keymatrix_initialise(&debug_cols[0], &debug_rows[0], KEYMODE_COL_ONLY);
}

/**
 * Service required peripherals for debug.
 */
void debug_service(bool first) {
    keymatrix_service();
}

/**
 * Handle the debug module during the setup state, just the ready button.
 * 
 * @param first true if it is the first time its been called this game
 */
void debug_game_setup(bool first) {
    if (first) {
        keymatrix_clear();
    }
    
    if (!tick_20hz) {
        return;
    }
    
    for (uint8_t press = keymatrix_fetch(); press != KEY_NO_PRESS; press = keymatrix_fetch()) {
        if (press & KEY_DOWN_BIT) {
            switch(press & KEY_NUM_BITS) {
                case 0: // Ready Up
                    game_module_ready(!this_module->ready);
                    break;
            }
        }
    }    
}

/**
 * Handle the debug module during the running state, strike and solved button.
 * 
 * @param first true if it is the first time its been called this game
 */
void debug_game_running(bool first) {
    if (first) {
        keymatrix_clear();
    }

    if (!tick_20hz) {
        return;
    }
    
    for (uint8_t press = keymatrix_fetch(); press != KEY_NO_PRESS; press = keymatrix_fetch()) {
        if (press & KEY_DOWN_BIT) {
            switch(press & KEY_NUM_BITS) {
                case 1: // Strike
                    game_module_strike(1);
                    break;
                case 2: // Solved
                    game_module_solved(true);
                    break;
            }
        }
    }        
}