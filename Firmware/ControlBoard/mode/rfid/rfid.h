#ifndef RFID_H
#define RFID_H

#include "../../gpio.h"
#include "../mode.h"
extern const mode_state_func_t rfid_funcs[MODE_STATE_COUNT];

typedef struct {
} rfid_t;

#define GPIO_RFID_LED_RED GPIO_B0
#define GPIO_RFID_LED_YELLOW GPIO_B1
#define GPIO_RFID_LED_GREEN GPIO_B2

#define GPIO_RFID_MODULE_SS GPIO_A0
#define GPIO_RFID_MODULE_IRQ GPIO_A1
#define GPIO_RFID_MODULE_RESTO GPIO_A2

#endif