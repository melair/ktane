#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include "edgework.h"
#include "rng.h"
#include "peripherals/lcd.h"
#include "game.h"
#include "tick.h"
#include "buzzer.h"

#define EDGEWORK_RNG_MASK 0x1e4ab852

#define DEFAULT_EDGEWORK    5
#define SERIAL_LENGTH       6
#define TWOFA_LENGTH        6

#define SLOT_PREFIX_MASK        0b11100000
#define SLOT_PREFIX_NOTHING     0b00000000
#define SLOT_PREFIX_PORT        0b00100000
#define SLOT_PREFIX_INDICATOR   0b01000000
#define SLOT_PREFIX_BATTERY     0b01100000
#define SLOT_PREFIX_TWOFA       0b10000000
#define SLOT_PREFIX_SERIAL      0b11100000

uint8_t edgework_slots[MAX_EDGEWORK];
uint8_t edgework_serial[SERIAL_LENGTH];
bool edgework_serial_has_vowel;
uint8_t edgework_battery;
bool edgework_has_twofa = false;
uint8_t edgework_twofa[TWOFA_LENGTH] = {'0', '0', '0', '0', '0', '0'};
uint32_t edgework_twofa_seed;

void edgework_generate_2fa(void);

const uint8_t indicator_name[INDICATOR_MAX + 1][3] = {
    "SND",
    "CLR",
    "CAR",
    "IND",
    "FRQ",
    "SIG",
    "NSA",
    "MSA",
    "TRN",
    "BOB",
    "FRK",
};

const uint8_t port_name[PORT_MAX + 1][4] = {
    "DVI ",
    "PARA",
    "PS2 ",
    "RJ45",
    "SERI",
    "RCA ",
};

/**
 * Generate new edgework for the game.
 *
 * @param seed seed to generate edgework for
 * @param difficulty level to set edgework for
 */
void edgework_generate(uint32_t seed, uint8_t difficulty) {
    for (uint8_t i = 0; i < MAX_EDGEWORK; i++) {
        edgework_slots[i] = 0;
    }

    edgework_battery = 0;
    edgework_has_twofa = false;
    edgework_twofa_seed = seed;
    edgework_serial_has_vowel = false;

    /* Generate serial number. */
    for (uint8_t i = 0; i < (SERIAL_LENGTH - 1); i++) {
        /* 35, not 36, Y is never present. */
        uint8_t t = rng_generate8(&seed, EDGEWORK_RNG_MASK) % 35;

        /* If it would be Y, bump to Z.*/
        if (t == 34) {
            t++;
        }

        if (t < 10) {
            edgework_serial[i] = t + '0';
        } else {
            edgework_serial[i] = t - 10 + 'A';
        }

        switch (edgework_serial[i]) {
            case 'A':
            case 'E':
            case 'I':
            case 'O':
            case 'U':
                edgework_serial_has_vowel = true;
                break;
        }
    }

    edgework_serial[SERIAL_LENGTH - 1] = (rng_generate8(&seed, EDGEWORK_RNG_MASK) % 10) + '0';

    edgework_slots[0] = SLOT_PREFIX_SERIAL;

    /* Generate edgework. */
    for (uint8_t i = 1; i < DEFAULT_EDGEWORK; i++) {
        uint8_t type = rng_generate8(&seed, EDGEWORK_RNG_MASK);
        uint8_t t = rng_generate8(&seed, EDGEWORK_RNG_MASK);

        switch (type % 4) {
            case 0:
                t = t % (PORT_MAX + 1);
                edgework_slots[i] = SLOT_PREFIX_PORT | t;
                break;
            case 1:
                t = t % (INDICATOR_MAX + 1);
                edgework_slots[i] = SLOT_PREFIX_INDICATOR | t;

                /* 60% chance the indicator is lit. */
                if ((rng_generate8(&seed, EDGEWORK_RNG_MASK) % 5) > 1) {
                    edgework_slots[i] |= 0b00010000;
                }

                break;
            case 2:
                t = t % 2;
                edgework_slots[i] = SLOT_PREFIX_BATTERY | t;
                edgework_battery += (t + 1);
                break;
            case 3:
                if (edgework_has_twofa) {
                    i--;
                } else {
                    edgework_slots[i] = SLOT_PREFIX_TWOFA;
                    edgework_has_twofa = true;
                }
        }
    }

    /* Shuffle edgework. */
    for (uint8_t i = 0; i < MAX_EDGEWORK; i++) {
        uint8_t s = rng_generate8(&seed, EDGEWORK_RNG_MASK) % MAX_EDGEWORK;
        uint8_t t = edgework_slots[i];
        edgework_slots[i] = edgework_slots[s];
        edgework_slots[s] = t;
    }
}

/**
 * Service the edgework and change it based upon time if required, for example
 * 2fa numbers.
 */
bool edgework_twofa_regenerated_this_tick = false;

void edgework_service(void) {
    if (game.state == GAME_START && edgework_has_twofa && !edgework_twofa_regenerated_this_tick) {
        edgework_generate_2fa();
        edgework_twofa_regenerated_this_tick = true;
    }

    if (tick_20hz && game.state == GAME_RUNNING && edgework_has_twofa) {
        if (game.time_remaining.seconds == 0) {
            if (!edgework_twofa_regenerated_this_tick) {
                edgework_twofa_regenerated_this_tick = true;
                edgework_generate_2fa();
                buzzer_on_timed(BUZZER_DEFAULT_VOLUME, BUZZER_FREQ_A6_SHARP, 40);
            }
        } else {
            edgework_twofa_regenerated_this_tick = false;
        }
    }
}

void edgework_generate_2fa(void) {
    for (uint8_t i = 0; i < TWOFA_LENGTH; i++) {
        edgework_twofa[i] = '0' + (rng_generate8(&edgework_twofa_seed, EDGEWORK_RNG_MASK) % 10);
    }
}

/**
 * Check to see if the indicator specified has the lit state queried.
 *
 * @param ind indicator to check for
 * @param lit true if indicator must be lit
 * @return true if the indicator is present and has the state required
 */
bool edgework_indicator_present(indicator_t ind, bool lit) {
    for (uint8_t i = 0; i < MAX_EDGEWORK; i++) {
        if ((edgework_slots[i] & SLOT_PREFIX_INDICATOR) != SLOT_PREFIX_INDICATOR) {
            continue;
        }

        uint8_t indE = edgework_slots[i] & 0b00001111;
        bool litE = ((edgework_slots[i] & 0b000100000) == 0b000100000);

        if (indE == ind && litE == lit) {
            return true;
        }
    }

    return false;
}

/**
 * Get the battery count.
 *
 * @return total battery count
 */
uint8_t edgework_battery_count(void) {
    return edgework_battery;
}

/**
 * Check if serial has a vowel.
 *
 * @return  true if serial has a vowel
 */
bool edgework_serial_vowel(void) {
    return edgework_serial_has_vowel;
}

/**
 * Get the last digit from serial number.
 *
 * @return last digit of serial number, as number not ASCII
 */
uint8_t edgework_serial_last_digit(void) {
    return edgework_serial[SERIAL_LENGTH - 1] - '0';
}

/**
 * Get the specified digit from the two fa code.
 *
 * @return requested digit of the two fa code, as number not ASCII
 */
uint8_t edgework_twofa_digit(uint8_t digit) {
    return edgework_twofa[digit] - '0';
}

/**
 * Check if the two fa edgework is present.
 *
 * @return true if the two fa edgework is present.
 */
bool edgework_twofa_present(void) {
    return edgework_has_twofa;
}

/**
 * Check to see if the port specified is present.
 *
 * @param port port to look for
 * @return true if port is present
 */
bool edgework_port_present(port_t port) {
    for (uint8_t i = 0; i < MAX_EDGEWORK; i++) {
        if ((edgework_slots[i] & SLOT_PREFIX_PORT) != SLOT_PREFIX_PORT) {
            continue;
        }

        if ((edgework_slots[i] & ~(SLOT_PREFIX_PORT)) == port) {
            return true;
        }
    }

    return false;
}

uint8_t edgework_count(void) {
    return MAX_EDGEWORK;
}

void edgework_display(uint8_t i) {
    lcd_clear();

    uint8_t ew = edgework_slots[i];

    const uint8_t *battery = "Battery:";
    const uint8_t *port = "Port:";
    const uint8_t *indLab = "Ind:";
    const uint8_t *indLit = "LIT";
    const uint8_t *twofa = "2FA:";
    const uint8_t *serial = "Serial:";

    uint8_t portNo;
    uint8_t ind;
    uint8_t batteryCount;

    switch (ew & SLOT_PREFIX_MASK) {
        case SLOT_PREFIX_PORT:
            lcd_update(1, 0, 5, port);

            portNo = ew & 0b00001111;
            lcd_update(1, 6, 4, &port_name[portNo]);

            break;
        case SLOT_PREFIX_INDICATOR:
            ind = ew & 0b00001111;
            lcd_update(1, 0, 4, indLab);
            lcd_update(1, 5, 3, &indicator_name[ind]);

            if ((ew & 0b00010000) == 0b00010000) {
                lcd_update(1, 9, 3, indLit);
            }

            break;
        case SLOT_PREFIX_BATTERY:
            batteryCount = (ew & ~(SLOT_PREFIX_MASK)) + 1 + '0';
            lcd_update(1, 0, 8, battery);
            lcd_update(1, 9, 1, &batteryCount);

            break;
        case SLOT_PREFIX_TWOFA:
            lcd_update(1, 0, 4, twofa);
            lcd_update(1, 5, TWOFA_LENGTH, &edgework_twofa);

            break;
        case SLOT_PREFIX_SERIAL:
            lcd_update(1, 0, 7, serial);
            lcd_update(1, 8, SERIAL_LENGTH, &edgework_serial);

            break;
        default:
            break;
    }

    lcd_sync();
}
