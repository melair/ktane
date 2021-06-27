#include <xc.h>
#include <stdbool.h>
#include <stdint.h>
#include "argb.h"
#include "mode.h"
#include "nvm.h"
#include "tick.h"
#include "modes/blank/blank.h"
#include "modes/bootstrap/bootstrap.h"

/* Local function prototypes. */
bool mode_check_if_bootstrap(void);

/* Modules configured mode*/
uint8_t configured_mode;

/**
 * Checks to see if the module is in bootstrap mode, this is done by shorting
 * pins A0 and A1 together. 
 * 
 * @return true if module is in bootstrap
 */
bool mode_check_if_bootstrap(void) {
    /* Set Port A1 (RA1) to be an input. */
    TRISAbits.TRISA1 = 1;
    
    tick_wait(1);
       
    /* Check to see if the port is high, if so abort. */
    if (PORTAbits.RA1 == 1) {
        TRISAbits.TRISA1 = 0;
        return false;
    }
    
    /* Set Port A1 (RA0) to high. */
    LATAbits.LATA0 = 1;
    
    tick_wait(1);    
    
    /* Check to see if the port now low, if so abort. */
    if (PORTAbits.RA1 == 0) {
        TRISAbits.TRISA1 = 0;
        LATAbits.LATA0 = 0;
        return false;
    }
    
    /* Set Port A1 (RA0) to low. */
    LATAbits.LATA0 = 0;
    
    tick_wait(1);    
    
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
 * Run any initialisation functions for the specific mode, an unrecognised mode
 * will be treated as a blank module and error and halt.
 */
void mode_initialise(void) {       
    configured_mode = nvm_read(EEPROM_LOC_MODE_CONFIGURATION);

    if (mode_check_if_bootstrap()) {
        configured_mode = MODE_BOOTSTRAP;
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
            break;
    }
}

/**
 * Service the mode.
 */
void mode_service(void) {
    switch(configured_mode) {
        case MODE_CONTROLLER:
            break;
    }
}