#include "pwm/pwm.h"
#include "pwm/pwm_platform.h"

bool PWM_Setup(const GPIO_PinDef *pin) {
    return PWM_PlatformSetup(pin);
}

void PWM_SetDuty(const GPIO_PinDef *pin, const uint8_t percent) {
    PWM_PlatformSetDuty(pin, percent);
}
