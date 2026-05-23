#ifndef TIMER_H
#define TIMER_H

#include "../../gpio.h"
#include "../mode.h"
#include <hal/argb.h>
#include <hal/spi.h>
#include <hal/i2c.h>

extern const mode_state_func_t timer_funcs[MODE_STATE_COUNT];

#define TIMER_ARGB_COUNT 47

typedef struct {
    argb_led_t argb_buffer[ARGB_DEFAULT_BUFFER_SIZE + TIMER_ARGB_COUNT];
    spi_transaction_t spi;
    i2c_transaction_t i2c;
} timer_t;

#define GPIO_TIMER_STRIKES_BLANK GPIO_C0
#define GPIO_TIMER_STRIKES_LATCH GPIO_C1

#define GPIO_TIMER_VOLUME_A GPIO_B0
#define GPIO_TIMER_VOLUME_B GPIO_B1
#define GPIO_TIMER_VOLUME_ACT GPIO_B2

#define TIMER_I2C_DIMMER_DIGPOT_LEFT 0b01011110
#define TIMER_I2C_DIMMER_DIGPOT_RIGHT 0b01011000

#endif