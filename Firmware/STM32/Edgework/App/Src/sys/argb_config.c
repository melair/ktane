#include "argb/argb_platform.h"

#include "sys/gpio.h"
#include "stm32g0xx_hal.h"

static void enable_timer_clock(void) {
    __HAL_RCC_TIM15_CLK_ENABLE();
}

const ARGB_Hardware ARGB_HARDWARE = {
    .timer_instance = TIM15,
    .timer_channel = TIM_CHANNEL_2,
    .timer_clock_hz = 64000000U,
    .data = {ARGB_Port, ARGB_Pin},
    .data_alternate = GPIO_AF5_TIM15,
    .dma_instance = DMA1_Channel2,
    .dma_request = DMA_REQUEST_TIM15_CH2,
    .dma_irq = DMA1_Channel2_3_IRQn,
    .enable_timer_clock = enable_timer_clock,
};

void DMA1_Channel2_3_IRQHandler(void) {
    ARGB_Platform_DMA_IRQHandler();
}
