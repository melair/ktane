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

/* Define the key matrix. */
volatile uint8_t *debug_col_ports[] = { &KPORTA,        &KPORTA,        &KPORTA,        NULL };
uint8_t debug_col_mask[]            = { 0b00100000,     0b01000000,     0b10000000,     0b00000000 };
volatile uint8_t *debug_row_ports[] = { NULL, };
uint8_t debug_row_mask[]            = { 0b00000000, };
uint8_t debug_key_state[3];


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
    mode_register_callback(GAME_SETUP, debug_game_setup);
    mode_register_callback(GAME_RUNNING, debug_game_running);
    
    /* Initialise keymatrix. */
    keymatrix_initialise(debug_col_ports, debug_col_mask, debug_col_mask, debug_row_ports, debug_row_mask, debug_key_state);
}

/**
 * Service required peripherals for debug.
 */
void debug_service(void) {
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