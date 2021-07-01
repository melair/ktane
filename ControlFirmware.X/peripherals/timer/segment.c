#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include "segment.h"
#include "../ports.h"

/*
 * Character Bank
 */
const uint8_t characters[24] = { 
    0b11111111, // SPACE
    0b01010000, // 0
    0b01011111, // 1
    0b00111000, // 2
    0b00011001, // 3    
    0b00010111, // 4
    0b10010001, // 5   
    0b10010000, // 6
    0b01011101, // 7
    0b00010000, // 8
    0b00010001, // 9
    0b00010100, // A
    0b10010010, // b
    0b11110000, // C
    0b00011010, // d
    0b10110000, // E
    0b10110100, // F
    0b11111101, // ?
    0b10111111, // -
    0b11111011, // _   
    0b10111001, // ?
    0b01010110, // ?
    0b11110110, // |
    0b11101111, // .
};

#define DIGIT0  0b00010000
#define DIGIT1  0b01000000
#define DIGIT2  0b10000000
#define DIGIT3  0b00001000
#define COLON   0b00100000

#define DIGIT_COUNT 5

uint8_t digits[DIGIT_COUNT] = { 0x00, 0x00, 0x00, 0x00, 0xff };
const uint8_t select[DIGIT_COUNT] = { DIGIT0, DIGIT1, DIGIT2, DIGIT3, COLON };
uint8_t digit = 0;

/* Local function prototypes. */
void segment_interrupt(void);

/**
 * Initialise the segment display, assumed to be in KPORTA and KPORTB.
 */
void segment_initialise(void) {
    KTRISA = 0x00;
    KTRISB &= 0b00000111;
    
    /* Set clock to 500kHz source. */
    T4CLKCONbits.CS = 0b00101;
    
    /* Set prescaler to 2, to 250kHz. */
    T4CONbits.CKPS = 0b001;
    
    /* Set period to 125, 2kHz. */
    T4PR = 125;
    
    /* Set timer to software gate, free running. */
    T4HLTbits.MODE = 0;
    
    /* Enable interrupt. */
    PIE11bits.TMR4IE = 1;
    /* Set interrupt to high priority.*/
    IPR11bits.TMR4IP = 1;
    
    /* Switch on timer. */
    T4CONbits.ON = 1;
}

/**
 * Handle update from timing for LED multiplexing.
 */
void __interrupt(irq(TMR4),base(8)) segment_interrupt(void) {
    PIR11bits.TMR4IF = 0;

    KLATA = digits[digit];
    KLATB = (KLATB & 0b00000111) | select[digit];

    digit++;
    if (digit >= DIGIT_COUNT) {
        digit = 0;
    }        
}