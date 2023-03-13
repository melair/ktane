#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include "pwmled.h"
#include "../hal/pins.h"

/* Storage of pins that handle LED cathodes. */
pin_t pwmled_rgb[3];

/* Structure for storing the state of each LED. */
typedef struct {
    uint16_t r;
    uint16_t g;
    uint16_t b;

    pin_t ch;
} pwmled_t;

/* Set and config of LEDs. */
#define MAX_LEDS 4
pwmled_t pwmled_leds[MAX_LEDS];

/* Where in the timer loop are we. */
uint16_t timer_pos = 0;
/* Which channel is currently being output. */
uint8_t on_ch = 0;

/* Local function prototypes. */
uint16_t pwmled_scale(uint8_t bri, uint8_t c);

/**
 * Initialise the PWM LED driver, expects switched high common annode, and
 * seperate RGB cathodes.
 *
 * @param r pin for red
 * @param g pin for green
 * @param b pin for blue
 * @param ch array of pins for annodes
 */
void pwmled_initialise(pin_t r, pin_t g, pin_t b, pin_t *ch) {
    pwmled_rgb[0] = r;
    pwmled_rgb[1] = g;
    pwmled_rgb[2] = b;

    for (uint8_t i = 0; i < MAX_LEDS; i++) {
        pwmled_leds[i].r = 0;
        pwmled_leds[i].g = 0;
        pwmled_leds[i].b = 0;
        pwmled_leds[i].ch = KPIN_NONE;
    }

    for (uint8_t i = 0; i < MAX_LEDS && ch[i] != KPIN_NONE; i++) {
        pwmled_leds[i].ch = ch[i];
    }

    /* Configure Timer1 to use FOSC, 64MHz. */
    T1CLKbits.CS = 0b00010;
    /* Prescaler of 1:2, resulting in 32MHz. */
    T1CONbits.CKPS = 0b01;
    /* Enable 16 bit mode. */
    T1CONbits.RD16 = 1;
    /* Initial period of 0xFFFF, causing an immediate overflow. */
    TMR1H = 0xff;
    TMR1L = 0xff;
    /* Switch on timer. */
    T1CONbits.ON = 1;
    /* Enable interrupt, will wake from sleep. */
    PIE3bits.TMR1IE = 1;
}

/**
 * Set one of the output LEDs to the following colour.
 *
 * @param ch led channel to set
 * @param r red value
 * @param g green value
 * @param b blue value
 */
void pwmled_set(uint8_t ch, uint8_t bri, uint8_t r, uint8_t g, uint8_t b) {
    pwmled_leds[ch].r = pwmled_scale(bri, r);
    pwmled_leds[ch].g = pwmled_scale(bri, g);
    pwmled_leds[ch].b = pwmled_scale(bri, b);
}

/**
 * Scale an requested value to a specified brightness.
 *
 * @param bri desired brightness
 * @param c unscaled value
 * @return scaled value
 */
uint16_t pwmled_scale(uint8_t bri, uint8_t c) {
    if (c == 0) {
        return 0;
    }

    uint16_t t = (c << 8) | 0xff;
    t = (t / 31) * bri;

    return t;
}

/**
 * Service the PWM loop, this works by setting the TIMER1 peripheral position
 * to various points.
 */
void pwmled_service(void) {
    if (!PIR3bits.TMR1IF) {
        return;
    }

    PIR3bits.TMR1IF = 0;

    /* Handle wrap around. */
    if (timer_pos == 0xffff) {
        timer_pos = 0;

        kpin_write(pwmled_leds[on_ch].ch, 1);

        on_ch++;
        if (on_ch == 4) {
            on_ch = 0;
        } else if (pwmled_leds[on_ch].ch == KPIN_NONE) {
            on_ch = 0;
        }

        kpin_write(pwmled_leds[on_ch].ch, 0);

        if (pwmled_leds[on_ch].r) {
            kpin_write(pwmled_rgb[0], 1);
        }

        if (pwmled_leds[on_ch].g) {
            kpin_write(pwmled_rgb[1], 1);
        }

        if (pwmled_leds[on_ch].b) {
            kpin_write(pwmled_rgb[2], 1);
        }
    }

    if (pwmled_leds[on_ch].r <= timer_pos) {
        kpin_write(pwmled_rgb[0], 0);
    }

    if (pwmled_leds[on_ch].g <= timer_pos) {
        kpin_write(pwmled_rgb[1], 0);
    }

    if (pwmled_leds[on_ch].b <= timer_pos) {
        kpin_write(pwmled_rgb[2], 0);
    }

    /* Find next duty. */
    uint16_t next_pos = 0xffff;

    if (pwmled_leds[on_ch].r > timer_pos && pwmled_leds[on_ch].r < next_pos) {
        next_pos = pwmled_leds[on_ch].r;
    }

    if (pwmled_leds[on_ch].g > timer_pos && pwmled_leds[on_ch].g < next_pos) {
        next_pos = pwmled_leds[on_ch].g;
    }

    if (pwmled_leds[on_ch].b > timer_pos && pwmled_leds[on_ch].b < next_pos) {
        next_pos = pwmled_leds[on_ch].b;
    }

    /* Calculate the number of Hz we need to wait until we're at the target Hz. */
    uint16_t wait_duration = 0xffff - (next_pos - timer_pos);

    /* Delta timer Hz, and reset clock. */
    TMR1H = (wait_duration >> 8) & 0xff;
    TMR1L = wait_duration & 0xff;

    /* Update our total clock, to track progress through the refresh. */
    timer_pos = next_pos;
}