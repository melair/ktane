#ifndef SIMON_H
#define SIMON_H

#include "../../gpio.h"
#include "../mode.h"
#include <hal/pin.h>
#include <hal/keymatrix.h>

void simon_common_service(void);
extern const mode_state_func_t simon_funcs[MODE_STATE_COUNT];

#define SIMON_BUTTON_COUNT 4

typedef struct {
    keymatrix_t keymatrix;
    keymatrix_state_t keymatrix_state[SIMON_BUTTON_COUNT];
    pin_t keymatrix_pins[SIMON_BUTTON_COUNT + 1];
} simon_t;

#define GPIO_SIMON_LED_BLUE GPIO_A0
#define GPIO_SIMON_LED_YELLOW GPIO_A1
#define GPIO_SIMON_LED_GREEN GPIO_A2
#define GPIO_SIMON_LED_RED GPIO_A3

#define GPIO_SIMON_BUTTON_BLUE GPIO_A4
#define GPIO_SIMON_BUTTON_YELLOW GPIO_A5
#define GPIO_SIMON_BUTTON_GREEN GPIO_A6
#define GPIO_SIMON_BUTTON_RED GPIO_A7

#endif