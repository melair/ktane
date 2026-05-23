#ifndef WHOSONFIRST_H
#define WHOSONFIRST_H

#include "../../gpio.h"
#include "../mode.h"

void whosonfirst_common_service(void);
extern const mode_state_func_t whosonfirst_funcs[MODE_STATE_COUNT];

typedef struct {
} whosonfirst_t;

#define GPIO_WHOSONFIRST_VFD_EN GPIO_A0
#define GPIO_WHOSONFIRST_VFD_RESET GPIO_A1
#define GPIO_WHOSONFIRST_VFD_CS GPIO_A2

#define GPIO_WHOSONFIRST_EPAPER_CS GPIO_B0
#define GPIO_WHOSONFIRST_EPAPER_DC GPIO_B1
#define GPIO_WHOSONFIRST_EPAPER_RES GPIO_B2
#define GPIO_WHOSONFIRST_EPAPER_BUSY GPIO_B3

#define GPIO_WHOSONFIRST_TOUCH_RESET GPIO_C0
#define GPIO_WHOSONFIRST_TOUCH_INT GPIO_C1

#define TOUCHPANEL_I2C_ADDR 0b01110000

#endif