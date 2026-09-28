#ifndef WHOSONFIRST_TOUCH_H
#define WHOSONFIRST_TOUCH_H

#include <stdbool.h>
#include <stdint.h>

#include "fsm/fsm.h"
#include "i2c/i2c.h"
#include "input_manager/input_manager.h"
#include "sys/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/* FT6336U uses a 7-bit I2C address; I2C_Queue supplies the R/W bit. */
#define TOUCH_FT6336_ADDRESS 0x38U

typedef struct {
    GPIO_PinDef reset;
    GPIO_PinDef interrupt;
    uint8_t address;
    IM_EventQueue *queue;
} Touch_Config;

typedef struct {
    Touch_Config config;
    FSM fsm;
    I2C_Transaction transaction;
    uint8_t register_address;
    uint8_t report[5];
    uint16_t last_x;
    uint16_t last_y;
    bool touch_active;
    bool initialized;
} Touch;

/* Starts the asynchronous hardware-reset and interrupt-driven read sequence. */
bool Touch_Init(Touch *touch, const Touch_Config *config);

/* Advance the reset and read FSM. Call for the lifetime of the mode. */
void Touch_Service(Touch *touch);

#ifdef __cplusplus
}
#endif

#endif //WHOSONFIRST_TOUCH_H
