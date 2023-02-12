#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include "ports.h"

#define kpin_is_porta(PIN)      ((PIN & 0b00111000) == 0b00000000)
#define kpin_is_portb(PIN)      ((PIN & 0b00111000) == 0b00001000)
#define kpin_is_portc(PIN)      ((PIN & ) == 0b00010000)

#define KPIN_PORT_MASK          0b00111000
#define KPIN_PIN_MASK(PIN)      (1 << (PIN & 0b00000111))

/**
 * Provide configuration support for pins on KTANE ports.
 *
 * @param port port to change
 * @param mode INPUT or OUTPUT mode
 * @param pullup true if weak pull up should be enabled
 */
void kpin_mode(pin_t pin, uint8_t mode, bool pullup) {
    uint8_t mask = KPIN_PIN_MASK(pin);

    switch(pin & KPIN_PORT_MASK) {
        case 0b00000000:
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
        case 0b00001000:
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
        case 0b00010000:
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
    }
}

/**
 * Read from KTANE pin.
 *
 * @param pin to read from
 * @return  true if read value is high
 */
bool kpin_read(pin_t pin) {
    uint8_t mask = KPIN_PIN_MASK(pin);
    uint8_t val;

    switch(pin & KPIN_PORT_MASK) {
        case 0b00000000:
            val = KPORTA;
            break;
        case 0b00001000:
            val = KPORTB;
            break;
        case 0b00010000:
            val = KPORTC;
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
    uint8_t mask = KPIN_PIN_MASK(pin);

    switch(pin & KPIN_PORT_MASK) {
        case 0b00000000:
            if (value) {
                KLATA |= mask;
            } else {
                KLATA &= ~mask;
            }
            break;
        case 0b00001000:
            if (value) {
                KLATB |= mask;
            } else {
                KLATB &= ~mask;
            }
            break;
        case 0b00010000:
            if (value) {
                KLATC |= mask;
            } else {
                KLATC &= ~mask;
            }
            break;
    }
}
