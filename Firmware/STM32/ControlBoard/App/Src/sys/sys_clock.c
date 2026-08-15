#include "sys/sys_clock.h"

#include <stdint.h>

#include "stm32h5xx_hal.h"
#include "stm32h5xx_it.h"

#define SYS_CLOCK_TIM2_CLOCK_HZ 250000000U
#define SYS_CLOCK_FREQUENCY_HZ 1000000U
#define SYS_CLOCK_TIM2_PRESCALER ((SYS_CLOCK_TIM2_CLOCK_HZ / SYS_CLOCK_FREQUENCY_HZ) - 1U)

_Static_assert((SYS_CLOCK_TIM2_CLOCK_HZ % SYS_CLOCK_FREQUENCY_HZ) == 0U,
               "TIM2 clock must be divisible by the system clock frequency");

static TIM_HandleTypeDef sys_clock_timer = {0};

void SysClock_Init(void) {
    TIM_ClockConfigTypeDef clock_source_config = {0};

    __HAL_RCC_TIM2_CLK_ENABLE();

    sys_clock_timer.Instance = TIM2;
    sys_clock_timer.Init.Prescaler = SYS_CLOCK_TIM2_PRESCALER;
    sys_clock_timer.Init.CounterMode = TIM_COUNTERMODE_UP;
    sys_clock_timer.Init.Period = UINT32_MAX;
    sys_clock_timer.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    sys_clock_timer.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_Base_Init(&sys_clock_timer) != HAL_OK) {
        Error_Handler();
    }

    clock_source_config.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&sys_clock_timer, &clock_source_config) != HAL_OK) {
        Error_Handler();
    }

    __HAL_TIM_SET_COUNTER(&sys_clock_timer, 0U);
    if (HAL_TIM_Base_Start(&sys_clock_timer) != HAL_OK) {
        Error_Handler();
    }
}

uint32_t SysClock_GetUs(void) {
    return __HAL_TIM_GET_COUNTER(&sys_clock_timer);
}
