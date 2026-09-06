#include "argb/argb_platform.h"

#include "sys/gpio.h"
#include "stm32h5xx_hal.h"

static void enable_timer_clock(void) {
    __HAL_RCC_TIM4_CLK_ENABLE();
}

const ARGB_Hardware ARGB_HARDWARE = {
    .timer_instance = TIM4,
    .timer_channel = TIM_CHANNEL_1,
    .timer_clock_hz = 250000000U,
    .data = {ARGB_Port, ARGB_Pin},
    .data_alternate = GPIO_AF2_TIM4,
    .dma_instance = GPDMA1_Channel0,
    .dma_request = GPDMA1_REQUEST_TIM4_CH1,
    .dma_irq = GPDMA1_Channel0_IRQn,
    .enable_timer_clock = enable_timer_clock,
};

void GPDMA1_Channel0_IRQHandler(void) {
    ARGB_Platform_DMA_IRQHandler();
}
