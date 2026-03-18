#include <xc.h>
#include <stdint.h>

/* Configure vectored interrupts. */
void int_init(uint24_t base_address) {
    /* Disable lock on IVT base address. */
    IVTLOCK = 0x55;
    IVTLOCK = 0xAA;
    IVTLOCKbits.IVTLOCKED = 0x00;

    /* Set base address. */
    IVTBASEU = (uint8_t)(base_address >> 16);
    IVTBASEH = (uint8_t)(base_address >> 8);
    IVTBASEL = (uint8_t)(base_address >> 0);

    /* Enable lock on IVT base address. */
    IVTLOCK = 0x55;
    IVTLOCK = 0xAA;
    IVTLOCKbits.IVTLOCKED = 0x01;

    /* Enable prioritised interrupts. */
    INTCON0bits.IPEN = 1;

    /* Enable global interrupts. */
    INTCON0bits.GIEH = 1;
    INTCON0bits.GIEL = 1;
}
