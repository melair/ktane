/**
 * Provides support for driving a piezo sounder via PWM.
 *
 * USES: PWM2S1
 */
#include <xc.h>
#include <stdint.h>
#include "buzzer.h"
#include "tick.h"
#include "../common/nvm.h"
#include "../common/eeprom_addrs.h"

/* Tick to turn buzzer off. */
uint32_t buzzer_off_tick = 0;
uint8_t buzzer_volume = 7;

/**
 * Initialise buzzer pin, and PWM to support buzzer.
 */
void buzzer_initialise(void) {
    /* Load last buzzer volume from EEPROM. */
    buzzer_volume = nvm_eeprom_read(EEPROM_LOC_BUZZER_VOL);

    /* Set RB4 to output. */
    TRISBbits.TRISB4 = 0;

    /* Set RB4 to use PWM2SP1. */
    RB4PPS = 0x1a;

    /* Disable external reset. */
    PWM2ERS = 0;

    /* Set PWM clock to FSOC, 64MHz. */
    PWM2CLKbits.CLK = 0b00010;

    /* Disable auto load. */
    PWM2LDS = 1;

    /* Prescale to 1:128. */
    PWM2CPRE = 127;
}

/**
 * Turn buzzer on.
 *
 * @param volume volume of buzzer, 0 to 255
 * @param frequency frequency of buzzer in Hz
 */
void buzzer_on(uint8_t volume, uint16_t frequency) {
#ifndef __DEBUG
    PWM2CONbits.EN = 0;

    PWM2PR = (uint16_t) ((uint24_t) 500000 / (uint24_t) frequency);
    if (PWM2PR == 0) {
        return;
    }

    uint16_t max_vol = PWM2PR >> 1;
    uint16_t vol_unit = max_vol / 7;
    PWM2S1P1 = vol_unit * buzzer_volume;

    PWM2CONbits.EN = 1;

    buzzer_off_tick = 0;
#endif
}

uint8_t buzzer_get_volume(void) {
    return buzzer_volume;
}

void buzzer_set_volume(uint8_t new_vol) {
    buzzer_volume = new_vol & 0b00000111;
}

/**
 * Turn buzzer on, timed.
 *
 * @param duration amount of time
 */
void buzzer_on_timed(uint8_t volume, uint16_t frequency, uint16_t duration) {
    buzzer_on(volume, frequency);
    buzzer_off_tick = tick_value + duration;
}

/**
 * Turn buzzer off.
 */
void buzzer_off(void) {
    PWM2CONbits.EN = 0;

    buzzer_off_tick = 0;
}

/**
 * Service the buzzer, turn off the buzzer if its due.
 */
void buzzer_service(void) {
    if (buzzer_off_tick > 0) {
        if (tick_value > buzzer_off_tick) {
            buzzer_off();
        }
    }
}