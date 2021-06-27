#include <xc.h>
#include <stdbool.h>
#include "bootstrap.h"
#include "../../argb.h"
#include "../../nvm.h"
#include "../../mode.h"
#include "../../tick.h"

/* Local function prototypes. */
uint8_t bootstrap_port_map(uint8_t);

void bootstrap_initialise(void) {      
    /* Set Port A (RF01), Port B (RF) and Port C (RD) to input. */
    TRISA = 0x03;
    TRISF = 0xff;
    TRISD = 0xff;
    
    /* Set the pins above ground to pull up. */
    WPUA = 0xc0;
    WPUF = 0xf0;
    WPUD = 0xf0;
    
    /* Wait for voltage rise times. */
    tick_wait(50);
    
    /* Capture current values. */
    bool change_mode = (PORTAbits.RA6 == 0);
    bool change_can = (PORTAbits.RA7 == 0);
    uint8_t port_b = bootstrap_port_map(PORTF);
    uint8_t port_c = bootstrap_port_map(PORTD);
    
    /* Reset pins back to safe defaults. */
    TRISA = 0x00;   
    TRISF = 0x00;
    TRISD = 0x00;
    WPUA = 0x00;
    WPUF = 0x00;
    WPUD = 0x00;
    
    /* Check to see if CAN ID is being changed. */
    if (change_can && port_c > 0) {
        nvm_write(EEPROM_LOC_CAN_ID, port_c);
    }
    
    /* Check to see if mode is being changes. */
    if (change_mode) {
        nvm_write(EEPROM_LOC_MODE_CONFIGURATION, port_b);
    }
    
    /* Toggle between red and green for bootstrap. */
    while(true) {
        argb_set(0, 31, 0xff, 0x00, 0x00);
        argb_service();
        tick_wait(25);
        argb_set(0, 31, 0x00, 0xff, 0x00);
        argb_service();
        tick_wait(25);
    }
}

uint8_t bootstrap_port_map(uint8_t p) {
    p = (p & 0x0f) | ((~p) & 0xf0);
    
    p = (p & 0xf0) >> 4 | (p & 0x0f) << 4;
    p = (p & 0xcc) >> 2 | (p & 0x33) << 2;
    p = (p & 0xaa) >> 1 | (p & 0x55) << 1;
    
    return p;
}