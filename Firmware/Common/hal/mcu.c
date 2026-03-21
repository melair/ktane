#include <xc.h>

void arbiter_init(void) {
    /* Store current interrupt state. */
    uint8_t gie = INTCON0bits.GIE;

    /* Disable interrupts. */
    INTCON0bits.GIE = 0;

    /* Unlock arbiter priorities. */
    PRLOCK = 0x55;
    PRLOCK = 0xAA;
    PRLOCKbits.PRLOCKED = 0;

    DMA1PR = 0;
    DMA2PR = 0;
    DMA3PR = 1;
    DMA4PR = 1;

    #ifdef DMA5PR
    DMA5PR = 2;
    #endif
    #ifdef DMA6PR
    DMA6PR = 2;
    #endif
    #ifdef DMA7PR
    DMA7PR = 3;
    #endif
    #ifdef DMA8PR
    DMA8PR = 3;
    #endif

    ISRPR = 4;
    MAINPR = 5;
    SCANPR = 6;

    /* Lock arbiter priorities. */
    PRLOCK = 0x55;
    PRLOCK = 0xAA;
    PRLOCKbits.PRLOCKED = 1;

    /* Restore global interrupts. */
    INTCON0bits.GIE = gie;
}

/* Cause CPU to idle on sleep(), rather than actually sleep. This is often needed when peripherals depend on FOSC. */
inline void sleep_init(void) {
    CPUDOZEbits.IDLEN = 1;
}
