#ifndef PWM_PLATFORM_H
#define PWM_PLATFORM_H

#include <stdbool.h>
#include <stdint.h>

#include "sys/gpio.h"

bool PWM_PlatformSetup(const GPIO_PinDef *pin);

void PWM_PlatformSetDuty(const GPIO_PinDef *pin, uint8_t percent);

#endif //PWM_PLATFORM_H
