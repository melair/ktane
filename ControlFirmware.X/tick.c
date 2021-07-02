/**
 * Provides 10ms tick for systems to build around.
 * 
 * Uses: TIMER2
 */
#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include "tick.h"

/* Current millisecond of uptime module has, will wrap at roughly 4 hours and 40 minutes. */
volatile uint32_t tick_value = 0;

/**
 * Initialise the ticker to provide an asynchronous time, uses Timer1 ticking 
 * at 100.0Hz.
 */
void tick_initialise(void) {
    /* Set clock to 500kHz source. */
    T2CLKCONbits.CS = 0b00101;
    
    /* Set prescaler to 2, to 250kHz. */
    T2CONbits.CKPS = 0b001;
    
    /* Set postscaler to 10, as timer rolls ever 1ms. */
    T2CONbits.OUTPS = 0b1001;
    
    /* Set period to 250. */
    T2PR = 250;
    
    /* Set timer to software gate, free running. */
    T2HLTbits.MODE = 0;
    
    /* Disable interrupt. */
    PIE3bits.TMR2IE = 0;
    
    /* Switch on timer. */
    T2CONbits.ON = 1;
}

/**
 * Service the tick from the timer, this checks for the tick interrupt and
 * increases the counter.
 */
void tick_service(void) {
    if(PIR3bits.TMR2IF == 1) {
        PIR3bits.TMR2IF = 0;
        tick_value++;           
    }
}

/**
 * Block and wait the number of milliseconds provided.
 * 
 * @param delay number of milliseconds to wait
 */
void tick_wait(uint8_t delay) {
    uint32_t target = tick_value + delay;
    
    while (tick_value < target) {
        tick_service();
        CLRWDT();
    }
}