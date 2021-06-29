/**
 * Provides support for driving a piezo sounder via PWM.
 * 
 * USES: PWM2S1
 */
#include <xc.h>
#include <stdint.h>
#include "buzzer.h"
#include "tick.h"

uint24_t buzzer_off_tick = 0;

#define TOTAL_FREQUENCIES 3

const uint16_t frequencies[TOTAL_FREQUENCIES][2] = {
    {BUZZER_FREQ_A6_SHARP, 0x010c}, // A#6
    {BUZZER_FREQ_C7_SHARP, 0x00e1}, // C#7
    {BUZZER_FREQ_B8, 0x003f}, // B8
};

/**
 * Initialise buzzer pin, and PWM to support buzzer.
 */
void buzzer_initialise(void) {
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
    PWM2CONbits.EN = 0;
    PWM2PR = 0;

    for (uint8_t i = 0; i < TOTAL_FREQUENCIES; i++) {
        if (frequency <= frequencies[i][0]) {
            PWM2PR = frequencies[i][1];
            break;
        }
    }

    if (PWM2PR == 0) {
        return;
    }
   
    PWM2S1P1 = PWM2PR-1;

    PWM2CONbits.EN = 1;
    
    buzzer_off_tick = 0;
}

/**
 * Turn buzzer on, timed.
 * 
 * @param duration amount of time 
 */
void buzzer_on_timed(uint8_t volume, uint16_t frequency, uint16_t duration) {
    buzzer_on(volume, frequency);
    buzzer_off_tick = tick_fetch() + duration;
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
        if (tick_fetch() > buzzer_off_tick) {
            buzzer_off();            
        }
    }
}