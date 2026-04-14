#ifndef POWER_H
#define POWER_H

#include "hal/i2c.h"
#include <hal/pin.h>
#include <utils/fsm.h>

#define POT_I2C_BUFFER_LEN 2

typedef struct  {
    fsm_t fsm;

    pin_t module_detect;
    pin_t efuse_en;
    pin_t efuse_flt;

    uint8_t current_limit;

    uint8_t pot_i2c_buffer[POT_I2C_BUFFER_LEN];
    i2c_transaction_t pot_i2c;
} power_t;

void power_init(void);
void power_service(void);

#endif