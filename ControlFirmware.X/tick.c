/**
 * Provides 10ms tick for systems to build around.
 * 
 * Uses: TIMER2
 */
#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include "tick.h"

/* Current millisecond of uptime module has, will wrap at roughly 50 days,
 * long enough not to care. */
volatile uint32_t tick_value = 0;

/* 100Hz tick flag. */
volatile bool tick_100hz = false;
/* 1kHz tick flag. */
volatile bool tick_1khz = false;
/* 2kHz tick flag. */
volatile bool tick_2khz = false;

/* Internal tick counter, used to maintain above tick flags. */
volatile uint8_t internal_tick = 0;


/**
 * Initialise the ticker to provide an asynchronous time, uses Timer0 ticking 
 * at 2kHz.
 */
void tick_initialise(void) {
    /* Set clock to 500kHz source. */
    T0CON1bits.CS = 0b101;
    
    /* Set prescaler to 2, to 250kHz. */
    T0CON1bits.CKPS = 0b0001;
    
    /* Set to 8 bit timer with period. */
    T0CON0bits.MD16 = 0;
    
    /* Set period to 125. */
    TMR0H = 125;
    
    /* Enable interrupt, require to wake us from IDLE mode. */
    PIE3bits.TMR0IE = 1;
    
    /* Switch on timer. */
    T0CON0bits.EN = 1;
}

/**
 * Service the tick from the timer, this checks for the tick interrupt and
 * increments internal counter, maintains ticks and main time source.
 */
void tick_service(void) {
    tick_100hz = false;
    tick_1khz = false;
    tick_2khz = false;
    
    if(PIR3bits.TMR0IF == 1) {
        PIR3bits.TMR0IF = 0;
                
        tick_2khz = true;
        if (internal_tick % 2 == 0) {
            tick_1khz = true;            
            tick_value++;
        }
        
        if (internal_tick % 20 == 0) {
            tick_100hz = true;
            internal_tick = 0;
        }
        
        internal_tick++;                   
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