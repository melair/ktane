#include <xc.h>
#include <stdbool.h>
#include <stdint.h>
#include "argb.h"
#include "mode.h"
#include "nvm.h"
#include "tick.h"
#include "game.h"
#include "modes/blank/blank.h"
#include "modes/bootstrap/bootstrap.h"
#include "modes/controller/controller.h"
#include "modes/debug/debug.h"

/* Local function prototypes. */
bool mode_check_if_bootstrap(void);
void mode_unconfigured_state(bool first);

/* Modules configured mode*/
uint8_t configured_mode;

/* Function pointers to each stage. */
void (*mode_state_function[5])(bool);
/* Last called stage. */
uint8_t last_called_state = 0xff;

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
    
    for (uint8_t i = 0; i < 5; i++) {
        mode_state_function[i] = mode_unconfigured_state;
    }
    
    switch(configured_mode) {
        /* Module is completely blank, do nothing. */
        default:
        case MODE_BLANK:
            blank_initialise();
            break;
        /* Module is in bootstrap, A0/A1 shorted. */
        case MODE_BOOTSTRAP:
            bootstrap_initialise();
            break;
        /* Module has no mode, but has been bootstrapped. */
        case MODE_UNCONFIGURED:
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
    }
}

/**
 * Service the mode.
 */
void mode_service(void) {
    switch(configured_mode) {
        case MODE_CONTROLLER:
            controller_service();
            break;
    }
    
    bool first = last_called_state != game.state;
    last_called_state = game.state;        
    mode_state_function[game.state](first);
}

/**
 * Register the modes callback against a game state. This prevents duplication
 * of state switching in modules.
 * 
 * @param state to call function for
 * @param func function to call
 */
void mode_register_callback(uint8_t state, void (*func)(bool)) {
    mode_state_function[state] = func;
}