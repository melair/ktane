#include "time.h"
#include <xc.h>
#include <stdint.h>
#include <stdbool.h>

/* Current millisecond of uptime module has, will wrap at roughly 50 days,
 * long enough not to care. */
volatile uint32_t uptime_in_ms = 0;

/* Tick flags bit-pack periodic events to reduce RAM usage. */
volatile uint8_t tick_flags = 0;

/* Internal tick counter, used to maintain above tick flags. */
volatile uint16_t internal_tick = 0;
/* Last proxessed tick. */
volatile uint16_t processed_tick = 0;

/* Countdown divisors for deriving slower periodic ticks from the 2kHz base. */
static uint8_t countdown_1khz = 2;
static uint8_t countdown_100hz = 20;
static uint8_t countdown_20hz = 100;
static uint16_t countdown_2hz = 1000;
static uint16_t countdown_1hz = 2000;

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
        tick_flags |= TIME_TICK_2KHZ;

        countdown_1khz--;
        if (countdown_1khz == 0) {
            countdown_1khz = 2;
            tick_flags |= TIME_TICK_1KHZ;
            uptime_in_ms++;
        }

        countdown_100hz--;
        if (countdown_100hz == 0) {
            countdown_100hz = 20;
            tick_flags |= TIME_TICK_100HZ;
        }

        countdown_20hz--;
        if (countdown_20hz == 0) {
            countdown_20hz = 100;
            tick_flags |= TIME_TICK_20HZ;
        }

        countdown_2hz--;
        if (countdown_2hz == 0) {
            countdown_2hz = 1000;
            tick_flags |= TIME_TICK_2HZ;
        }

        countdown_1hz--;
        if (countdown_1hz == 0) {
            countdown_1hz = 2000;
            tick_flags |= TIME_TICK_1HZ;
            reset_internal = true;
        }
    }

    if (reset_internal) {
      internal_tick = 0;
      processed_tick = 0;
    }
}

bool time_service_end(void) {
    tick_flags = 0;
    return (internal_tick == processed_tick);
}
