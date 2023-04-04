#include <xc.h>

/**
 * Safe unused pins.
 */
void safe_unused_pins(void) {
    /* Disable analogue functionality. */
    ANSELA = 0;
    ANSELB = 0;
    ANSELC = 0;
    ANSELD = 0;
    ANSELE = 0;
    ANSELF = 0;

    /* Set all pins to output. */
    TRISA = 0;
    TRISB = 0;
    TRISC = 0;
    TRISD = 0;
    TRISE = 0;
    TRISF = 0;

    /* Drive all pins low. */
    LATA = 0;
    LATB = 0;
    LATC = 0;
    LATD = 0;
    LATE = 0;
    LATF = 0;
}

/**
 * Unlock PPS registers, assumes global interrupts are disabled.
 */
void pps_unlock(void) {
    PPSLOCK = 0x55;
    PPSLOCK = 0xAA;
    PPSLOCKbits.PPSLOCKED = 0;
}

/**
 * Lock PPS registers, assumes global interrupts are disabled.
 */
void pps_lock(void) {
    PPSLOCK = 0x55;
    PPSLOCK = 0xAA;
    PPSLOCKbits.PPSLOCKED = 1;
}

/**
 * Reconfigure the system arbiter, giving DMA1 and DMA2 priority over the
 * ISR and main CPU.
 */
void arbiter_initialise(void) {
    PRLOCK = 0x55;
    PRLOCK = 0xAA;
    PRLOCKbits.PRLOCKED = 0;

    DMA1PR = 0;
    DMA2PR = 1;
    DMA3PR = 2;
    DMA4PR = 3;
    ISRPR = 4;
    MAINPR = 5;
    SCANPR = 6;

    PRLOCK = 0x55;
    PRLOCK = 0xAA;
    PRLOCKbits.PRLOCKED = 1;
}
