#ifndef PWM_H
#define PWM_H

#include <stdbool.h>
#include <stdint.h>

#include "sys/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

bool PWM_Setup(const GPIO_PinDef *pin);

void PWM_SetDuty(const GPIO_PinDef *pin, uint8_t percent);

#ifdef __cplusplus
}
#endif

#endif //PWM_H
