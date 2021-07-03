#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include "segment.h"
#include "../ports.h"
#include "../../tick.h"

/*
 * Character Bank
 * 
 * Bits are not in any sane order, this is due to ease of electrical wiring.
 * 
 *      A
 *    F   B
 *      G
 *    E   C
 *      D     DP
 *
 * A  = 0b00000010
 * B  = 0b10000000
 * C  = 0b00100000
 * D  = 0b00000100
 * E  = 0b00000001
 * F  = 0b00001000
 * G  = 0b01000000
 * DP = 0b00010000
 */
const uint8_t characters[25] = { 
    0b00000000, // SPACE
    0b10101111, // 0
    0b10100000, // 1
    0b11000111, // 2
    0b11100110, // 3    
    0b11101000, // 4
    0b01101110, // 5   
    0b01101111, // 6 
    0b10100010, // 7
    0b11101111, // 8
    0b11101110, // 9
    0b11101011, // A
    0b01101101, // b
    0b00001111, // C
    0b11100101, // d
    0b01001111, // E
    0b01001011, // F
    0b00000010, // OverScore
    0b01000000, // Dash
    0b00000100, // Underscore
    0b01000110, // Three Horizontal
    0b00000101, // Top/Bottom
    0b10101001, // ||
    0b00001001, // |
    0b00010000, // .
};

#define DIGIT0  0b00010000
#define DIGIT1  0b01000000
#define DIGIT2  0b10000000
#define DIGIT3  0b00001000
#define COLON   0b00100000

#define DIGIT_COUNT 5

uint8_t digits[DIGIT_COUNT] = { 0x00, 0x00, 0x00, 0x00, 0x00 };
const uint8_t select[DIGIT_COUNT] = { DIGIT0, DIGIT1, DIGIT2, DIGIT3, COLON };
uint8_t digit = 0;
bool colon = false;

/**
 * Initialise the segment display, assumed to be in KPORTA and KPORTB.
 */
void segment_initialise(void) {
    KTRISA = 0x00;
    KTRISB &= 0b00000111;
}

/**
 * Service Timer4 to drive multiplexed LED display.
 */
void segment_service(void) {
    if (tick_2khz) {
        /* Inverse the digit bits, as a high switches off the low side switch. */
        KLATA = ~digits[digit];
        
        /* Handle colon. */
        if (digit == 4 && !colon) {
            KLATB = (KLATB & 0b00000111);
        } else {
            KLATB = (KLATB & 0b00000111) | select[digit];            
        }

        digit++;
        if (digit >= DIGIT_COUNT) {
            digit = 0;
        } 
    }
}

/**
 * Set a segment digit.
 * 
 * @param digit digit number to set
 * @param value value to display
 */
void segment_set_digit(uint8_t digit, uint8_t value) {
    if (digit >= DIGIT_COUNT) {
        return;
    }
    
    digits[digit] = value;
}

/**
 * Set colon to be on or off.
 * 
 * @param on true if colon is to be on
 */
void segment_set_colon(bool on) {
    colon = on;
}