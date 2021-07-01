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
volatile uint8_t tick_interrupts = 0;

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
    
    /* Enable interrupt. */
    PIE3bits.TMR2IE = 1;
    /* Set interrupt to high priority.*/
    IPR3bits.TMR2IP = 1;
    
    /* Switch on timer. */
    T2CONbits.ON = 1;
}

/**
 * Handle Timer1 interrupt to increase tick_period.
 */
void __interrupt(irq(TMR2),base(8)) tick_interrupt(void) {
    /* Tick will overflow to 0 on it's own. */
    tick_interrupts++;

    PIR3bits.TMR2IF = 0;
}

/**
 * Service the tick from the timer, this transfers stored interrupt counts and
 * adds it to the tick value. This ensures that one run of the main service loop
 * uses the same time and does not change mid flow.
 */
void tick_service(void) {
    while(PIR3bits.TMR2IF == 1);
    PIE3bits.TMR2IE = 0;
    tick_value += tick_interrupts;
    tick_interrupts = 0;
    PIE3bits.TMR2IE = 1;
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