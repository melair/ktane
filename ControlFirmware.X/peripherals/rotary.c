#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include "rotary.h"
#include "../tick.h"

uint8_t rotary_next_code = 0;
uint16_t rotary_state = 0;

int8_t rotary_delta = 0;

pin_t rotary_clk;
pin_t rotary_dat;

void rotary_initialise(pin_t a, pin_t b) {
    rotary_clk = a;
    rotary_dat = b;

    kpin_mode(rotary_clk, PIN_INPUT, true);
    kpin_mode(rotary_dat, PIN_INPUT, true);
}

void rotary_service(void) {
    if (!tick_2khz) {
        return;
    }

    static int8_t rot_enc_table[] = {0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0};

    rotary_next_code <<= 2;
    if (kpin_read(rotary_dat)) {
        rotary_next_code |= 0x02;
    }

    if (kpin_read(rotary_clk)) {
        rotary_next_code |= 0x01;
    }

    rotary_next_code &= 0x0f;

    if (rot_enc_table[rotary_next_code]) {
        rotary_state <<= 4;
        rotary_state |= rotary_next_code;
        if ((rotary_state & 0xff) == 0x2b) {
            rotary_delta--;
        }
        if ((rotary_state & 0xff) == 0x17) {
            rotary_delta++;
        }
    }
}

int8_t rotary_fetch_delta(void) {
    return rotary_delta;
}

void rotary_clear(void) {
    rotary_delta = 0;
}
