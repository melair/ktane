#include "time.h"
#include <xc.h>
#include <stdint.h>
#include <stdbool.h>

/* Current millisecond of uptime module has, will wrap at roughly 50 days,
 * long enough not to care. */
volatile uint32_t uptime = 0;

/* 1Hz tick flag. */
volatile bool tick_1hz = false;
/* 2Hz tick flag. */
volatile bool tick_2hz = false;
/* 20Hz tick flag. */
volatile bool tick_20hz = false;
/* 100Hz tick flag. */
volatile bool tick_100hz = false;
/* 1kHz tick flag. */
volatile bool tick_1khz = false;
/* 2kHz tick flag. */
volatile bool tick_2khz = false;

/* Internal tick counter, used to maintain above tick flags. */
volatile uint16_t internal_tick = 0;
/* Last proxessed tick. */
volatile uint16_t processed_tick = 0;

void time_init(void) {
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

void time_interrupt(void) {
  PIR3bits.TMR0IF = 0;
  internal_tick++;
}

void time_service_start(void) {
    if (internal_tick == processed_tick) {
        return;
    }

    uint32_t diff = internal_tick - processed_tick;
    bool reset_internal = false;

    for (uint32_t i = 0; i < diff; i++) {
        processed_tick++;
        tick_2khz = true;

        if (processed_tick % 2 == 0) {
            tick_1khz = true;
            uptime++;
        }

        if (processed_tick % 20 == 0) {
            tick_100hz = true;
        }

        if (processed_tick % 100 == 0) {
            tick_20hz = true;
        }

        if (processed_tick % 1000 == 0) {
            tick_2hz = true;
        }

        if (processed_tick % 2000 == 0) {
            tick_1hz = true;
            reset_internal = true;
        }
    }

    if (reset_internal) {
      internal_tick = 0;
      processed_tick = 0;
    }
}

bool time_service_end(void) {
    tick_1hz = false;
    tick_2hz = false;
    tick_20hz = false;
    tick_100hz = false;
    tick_1khz = false;
    tick_2khz = false;

    return (internal_tick == processed_tick);
}