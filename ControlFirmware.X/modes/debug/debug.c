#include <xc.h>
#include <stdbool.h>
#include "debug.h"
#include "../../mode.h"
#include "../../game.h"
#include "../../tick.h"
#include "../../peripherals/ports.h"

/* Local function prototypes. */
void debug_game_setup(bool first);
void debug_game_running(bool first);

/**
 * Initialise the debug puzzle, a simple puzzle that allows declaration of ready,
 * make a strike or mark solved.
 */
void debug_initialise(void) {
    /* Make debug pins input. */
    KTRISAbits.TRISA5 = 1;
    KTRISAbits.TRISA6 = 1;
    KTRISAbits.TRISA7 = 1;
    
    /* Enable weak pull up, ground to activate. */
    KWPUAbits.WPUA5 = 1;
    KWPUAbits.WPUA6 = 1;
    KWPUAbits.WPUA7 = 1;
    
    /* Register our callbacks. */
    mode_register_callback(GAME_SETUP, debug_game_setup);
    mode_register_callback(GAME_RUNNING, debug_game_running);
}

/* Button polling period. */
#define CHECK_PERIOD 200
/* Last tick buttons were check during. */
uint32_t debug_last_tick = 0;

/* Previous state of ready button. */
bool ready_last;
/* Previous state of strike button. */
bool strike_last;
/* Previous state of solved button. */
bool solved_last;

/**
 * Handle the debug module during the setup state, just the ready button.
 * 
 * @param first true if it is the first time its been called this game
 */
void debug_game_setup(bool first) {
    if ((tick_value - debug_last_tick) < CHECK_PERIOD) {
        return;
    }
    
    debug_last_tick = tick_value;
         
    /* A5: Game Ready - Only active during GAME_SETUP. */
    bool ready_current = (!KPORTAbits.RA5);

    if (ready_current != ready_last) {
        ready_last = ready_current;

        if (ready_current) {
            game_module_ready(!this_module->ready);
        }
    }    
}

/**
 * Handle the debug module during the running state, strike and solved button.
 * 
 * @param first true if it is the first time its been called this game
 */
void debug_game_running(bool first) {
    if ((tick_value - debug_last_tick) < CHECK_PERIOD) {
        return;
    }
    
    debug_last_tick = tick_value;
         
    /* A6: Puzzle Strike - Only active during GAME_RUNNING. */
    bool strike_current = (!KPORTAbits.RA6);

    if (strike_current != ready_last) {
        ready_last = strike_current;

        if (strike_current) {
            game_module_strike(1);
        }            
    }  

    /* A7: Puzzle Solved - Only active during GAME_RUNNING. */
    bool solved_current = (!KPORTAbits.RA7);

    if (solved_current != solved_last) {
        solved_last = solved_current;

        if (solved_current) {
            game_module_solved(true);
        }            
    }     
}