#include <xc.h>
#include "tick.h"

/* Local prototype functions. */
void __interrupt(irq(default),base(0x108)) int_default(void);

/**
 * Initialise interrupt control, using vectored tables. This allows us to
 * not make function calls in the ISRs.
 */
void int_initialise(void) {
    /* Enable prioritised interrupts. */
    INTCON0bits.IPEN = 1;

    /* Disable lock on IVT base address. */
    IVTLOCK = 0x55;
    IVTLOCK = 0xAA;
    IVTLOCKbits.IVTLOCKED = 0x00;

    /* Set base address to 0x108. */
    IVTBASEU = 0x00;
    IVTBASEH = 0x01;
    IVTBASEL = 0x08;

    /* Enable lock on IVT base address. */
    IVTLOCK = 0x55;
    IVTLOCK = 0xAA;
    IVTLOCKbits.IVTLOCKED = 0x01;

    /* Disable global interrupts, this is required for waking from IDLE mode
     * without invoking an interrupt handler. */
    INTCON0bits.GIEH = 0;
    INTCON0bits.GIEL = 0;
}

/**
 * Default interrupt handler, should never be used.
 */
void __interrupt(irq(default),base(0x108)) int_default(void) {
}