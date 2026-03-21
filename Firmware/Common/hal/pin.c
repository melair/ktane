#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include "pin.h"

/* This code assumes that port registers are a standard interval in SFR space, this is
 * particular to the MCU. */
#define ANSEL_INTERVAL  (uint16_t)((uintptr_t)&ANSELB - (uintptr_t)&ANSELA)
#define ODCON_INTERVAL  (uint16_t)((uintptr_t)&ODCONB - (uintptr_t)&ODCONA)
#define WPU_INTERVAL    (uint16_t)((uintptr_t)&WPUB - (uintptr_t)&WPUA)
#define TRIS_INTERVAL   (uint16_t)((uintptr_t)&TRISB - (uintptr_t)&TRISA)
#define PORT_INTERVAL   (uint16_t)((uintptr_t)&PORTB - (uintptr_t)&PORTA)
#define LAT_INTERVAL    (uint16_t)((uintptr_t)&LATB - (uintptr_t)&LATA)

/* Use RB4/RA4, as RB0-RB3 don't exist on a PIC18F15Q40. */
#define RxyPPS_INTERVAL (uint16_t)((uintptr_t)&RB4PPS - (uintptr_t)&RA4PPS)

void pin_config(pin_t pin, uint8_t dir, uint8_t config) {
    uint8_t port_num = PORT_NUM(pin);
    uint8_t pin_mask = PIN_MASK(pin);

    volatile uint8_t *ansel = (volatile uint8_t *)(((uintptr_t)&ANSELA) + (ANSEL_INTERVAL * port_num));
    volatile uint8_t *odcon = (volatile uint8_t *)(((uintptr_t)&ODCONA) + (ODCON_INTERVAL * port_num));
    volatile uint8_t *wpu = (volatile uint8_t *)(((uintptr_t)&WPUA) + (WPU_INTERVAL * port_num));
    volatile uint8_t *tris = (volatile uint8_t *)(((uintptr_t)&TRISA) + (TRIS_INTERVAL * port_num));

    /* Set analogue mode. */
    if (config & CFG_ANALOG) {
        *ansel |= pin_mask;
    } else {
        *ansel &= ~pin_mask;
    }

    /* Set open-drain mode. */
    if (config & CFG_OPENDRAIN) {
        *odcon |= pin_mask;
    } else {
        *odcon &= ~pin_mask;
    }

    /* Set pull-up mode. */
    if (config & CFG_PULLUP) {
        *wpu |= pin_mask;
    } else {
        *wpu &= ~pin_mask;
    }

    /* Set direction. */
    if (dir == INPUT) {
        *tris |= pin_mask;
    } else {
        *tris &= ~pin_mask;
    }
}

void pin_write(pin_t pin, bool value) {
    uint8_t port_num = PORT_NUM(pin);
    uint8_t pin_mask = PIN_MASK(pin);

    volatile uint8_t *lat = (volatile uint8_t *)(((uintptr_t)&LATA) + (LAT_INTERVAL * port_num));

    if (value) {
        *lat |= pin_mask;
    } else {
        *lat &= ~pin_mask;
    }
}

bool pin_read(pin_t pin) {
    uint8_t port_num = PORT_NUM(pin);
    uint8_t pin_mask = PIN_MASK(pin);

    volatile uint8_t *port = (volatile uint8_t *)(((uintptr_t)&PORTA) + (PORT_INTERVAL * port_num));

    return (*port & pin_mask) == 1;
}

volatile uint8_t *pin_to_pps(pin_t pin) {
    uint8_t port_num = PORT_NUM(pin);
    uint8_t pin_num = PIN_NUM(pin);

    return (volatile uint8_t *)(((uintptr_t)&RA0PPS) + (RxyPPS_INTERVAL * port_num) + pin_num);
}
