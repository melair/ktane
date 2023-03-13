#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include "pins.h"

#define KPIN_PORT_MASK          0b11111000
#define KPIN_ALL_PIN_MASK       0b00000111

#define KPIN_PORT_A             0b00000000
#define KPIN_PORT_B             0b00001000
#define KPIN_PORT_C             0b00010000
#define KPIN_PORT_D             0b00011000

#define KPIN_SYS_B              0b10001000
#define KPIN_SYS_C              0b10010000

#define kpin_is_porta(PIN)      ((PIN & KPIN_PORT_MASK) == KPIN_PORT_A)
#define kpin_is_portb(PIN)      ((PIN & KPIN_PORT_MASK) == KPIN_PORT_B)
#define kpin_is_portc(PIN)      ((PIN & KPIN_PORT_MASK) == KPIN_PORT_C)
#define kpin_is_portd(PIN)      ((PIN & KPIN_PORT_MASK) == KPIN_PORT_D)

#define KPIN_PIN_MASK(PIN)      (1 << (PIN & 0b00000111))

/**
 * Provide configuration support for pins on KTANE ports.
 *
 * @param port port to change
 * @param mode INPUT or OUTPUT mode
 * @param pullup true if weak pull up should be enabled
 */
void kpin_mode(pin_t pin, uint8_t mode, bool pullup) {
    if (pin == KPIN_NONE) {
        return;
    }
    
    uint8_t mask = KPIN_PIN_MASK(pin);

    switch(pin & KPIN_PORT_MASK) {
        case KPIN_PORT_A:
            if (mode == PIN_OUTPUT) {
                KTRISA &= ~mask;
            } else {
                KTRISA |= mask;
            }

            if (pullup) {
                KWPUA |= mask;
            } else {
                KWPUA &= ~mask;
            }
            break;
        case KPIN_PORT_B:
            if (mode == PIN_OUTPUT) {
                KTRISB &= ~mask;
            } else {
                KTRISB |= mask;
            }

            if (pullup) {
                KWPUB |= mask;
            } else {
                KWPUB &= ~mask;
            }
            break;
        case KPIN_PORT_C:
            if (mode == PIN_OUTPUT) {
                KTRISC &= ~mask;
            } else {
                KTRISC |= mask;
            }

            if (pullup) {
                KWPUC |= mask;
            } else {
                KWPUC &= ~mask;
            }
            break;
        case KPIN_PORT_D:
            if (mode == PIN_OUTPUT) {
                KTRISD &= ~mask;
            } else {
                KTRISD |= mask;
            }

            if (pullup) {
                KWPUD |= mask;
            } else {
                KWPUD &= ~mask;
            }
            break;
    }
}

void kpin_opendrain(pin_t pin, bool opendrain) {
      if (pin == KPIN_NONE) {
        return;
    }
    
    uint8_t mask = KPIN_PIN_MASK(pin);

    switch(pin & KPIN_PORT_MASK) {
        case KPIN_PORT_A:
            if (opendrain) {
                KODCONA |= mask;
            } else {
                KODCONA &= ~mask;
            }
            break;
        case KPIN_PORT_B:
            if (opendrain) {
                KODCONB |= mask;
            } else {
                KODCONB &= ~mask;
            }
            break;
        case KPIN_PORT_C:
            if (opendrain) {
                KODCONC |= mask;
            } else {
                KODCONC &= ~mask;
            }
            break;
        case KPIN_PORT_D:
            if (opendrain) {
                KODCOND |= mask;
            } else {
                KODCOND &= ~mask;
            }
            break;
    }  
}

/**
 * Read from KTANE pin.
 *
 * @param pin to read from
 * @return  true if read value is high
 */
bool kpin_read(pin_t pin) {
    if (pin == KPIN_NONE) {
        return false;
    }
    
    uint8_t mask = KPIN_PIN_MASK(pin);
    uint8_t val;

    switch(pin & KPIN_PORT_MASK) {
        case KPIN_PORT_A:
            val = KPORTA;
            break;
        case KPIN_PORT_B:
            val = KPORTB;
            break;
        case KPIN_PORT_C:
            val = KPORTC;
            break;
        case KPIN_PORT_D:
            val = KPORTD;
            break;
    }

    return ((val & mask) == mask);
}

/**
 * Write to KTANE pin.
 *
 * @param pin to write to
 * @param value high or low value to write
 */
void kpin_write(pin_t pin, bool value) {
    if (pin == KPIN_NONE) {
        return;
    }
    
    uint8_t mask = KPIN_PIN_MASK(pin);

    switch(pin & KPIN_PORT_MASK) {
        case KPIN_PORT_A:
            if (value) {
                KLATA |= mask;
            } else {
                KLATA &= ~mask;
            }
            break;
        case KPIN_PORT_B:
            if (value) {
                KLATB |= mask;
            } else {
                KLATB &= ~mask;
            }
            break;
        case KPIN_PORT_C:
            if (value) {
                KLATC |= mask;
            } else {
                KLATC &= ~mask;
            }
            break;
        case KPIN_PORT_D:
            if (value) {
                KLATD |= mask;
            } else {
                KLATD &= ~mask;
            }
            break;
    }
}

volatile unsigned char *kpin_to_rxypps(pin_t pin) {
    uint8_t mask = pin & KPIN_ALL_PIN_MASK;

    switch(pin & KPIN_PORT_MASK) {
        case KPIN_PORT_A:
            return (&RA0PPS) + mask;
        case KPIN_PORT_B:
            return (&RF0PPS) + mask;
        case KPIN_PORT_C:
            return (&RD0PPS) + mask;
        case KPIN_PORT_D:
            return (&RC0PPS) + mask;
        case KPIN_SYS_B:
            return (&RB0PPS) + mask;
        case KPIN_SYS_C:
            return (&RC0PPS) + mask;
    } 
}

uint8_t kpin_to_ppspin(pin_t pin) {
    uint8_t mask = pin & KPIN_ALL_PIN_MASK;

    switch(pin & KPIN_PORT_MASK) {
        case KPIN_PORT_A:
            return (0b00000000 | mask);
        case KPIN_PORT_B:
            return (0b00101000 | mask);
        case KPIN_PORT_C:
            return (0b00011000 | mask);
        case KPIN_PORT_D:
            return (0b00010000 | mask);
        case KPIN_SYS_B:
            return (0b00001000 | mask);
        case KPIN_SYS_C:
            return (0b00010000 | mask);
    }   
}
