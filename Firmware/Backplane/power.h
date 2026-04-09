#ifndef POWER_H
#define POWER_H

#include <hal/pin.h>
#include <utils/fsm.h>

#define I2C_FRONT_POT_ADDR 0b0101111
#define I2C_REAR_POT_ADDR 0b0101100

typedef struct  {
    fsm_t fsm;

    pin_t module_detect;
    pin_t efuse_en;
    pin_t efuse_flt;

    uint8_t pot_i2c_address;
} power_t;

void power_init(void);
void power_service(void);

#endif