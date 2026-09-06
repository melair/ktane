#include "argb/argb_platform.h"

#include "stm32g0xx_hal.h"

static const ARGB_Hardware *argb_hardware;
static TIM_HandleTypeDef timer_handle;
static DMA_HandleTypeDef dma_handle;

static uint32_t dma_id_for_channel(const uint32_t channel) {
    switch (channel) {
        case TIM_CHANNEL_1:
            return TIM_DMA_ID_CC1;
        case TIM_CHANNEL_2:
            return TIM_DMA_ID_CC2;
        case TIM_CHANNEL_3:
            return TIM_DMA_ID_CC3;
        case TIM_CHANNEL_4:
            return TIM_DMA_ID_CC4;
        default:
            return UINT32_MAX;
    }
}

static bool hardware_valid(const ARGB_Hardware *hardware, const uint16_t period_ticks) {
    return (hardware != NULL) &&
           (hardware->timer_instance != NULL) &&
           (dma_id_for_channel(hardware->timer_channel) != UINT32_MAX) &&
           (hardware->timer_clock_hz != 0U) &&
           (hardware->data.port != NULL) &&
           (hardware->dma_instance != NULL) &&
           (hardware->enable_timer_clock != NULL) &&
           (period_ticks > 0U);
}

static bool init(const ARGB_Hardware *hardware, const uint16_t period_ticks) {
    if (!hardware_valid(hardware, period_ticks)) {
        return false;
    }

    argb_hardware = hardware;
    hardware->enable_timer_clock();

    GPIO_InitTypeDef gpio_init = {0};
    gpio_init.Pin = hardware->data.pin;
    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio_init.Alternate = hardware->data_alternate;
    HAL_GPIO_Init(hardware->data.port, &gpio_init);

    timer_handle.Instance = hardware->timer_instance;
    timer_handle.Init.Prescaler = 0U;
    timer_handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    timer_handle.Init.Period = period_ticks - 1U;
    timer_handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    timer_handle.Init.RepetitionCounter = 0U;
    timer_handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&timer_handle) != HAL_OK) {
        return false;
    }

    TIM_ClockConfigTypeDef clock_config = {0};
    clock_config.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if ((HAL_TIM_ConfigClockSource(&timer_handle, &clock_config) != HAL_OK) ||
        (HAL_TIM_PWM_Init(&timer_handle) != HAL_OK)) {
        return false;
    }

    TIM_MasterConfigTypeDef master_config = {0};
    master_config.MasterOutputTrigger = TIM_TRGO_RESET;
    master_config.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&timer_handle, &master_config) != HAL_OK) {
        return false;
    }

    TIM_OC_InitTypeDef output_config = {0};
    output_config.OCMode = TIM_OCMODE_PWM1;
    output_config.Pulse = 0U;
    output_config.OCPolarity = TIM_OCPOLARITY_HIGH;
    output_config.OCNPolarity = TIM_OCNPOLARITY_HIGH;
    output_config.OCFastMode = TIM_OCFAST_DISABLE;
    output_config.OCIdleState = TIM_OCIDLESTATE_RESET;
    output_config.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    if (HAL_TIM_PWM_ConfigChannel(&timer_handle, &output_config, hardware->timer_channel) != HAL_OK) {
        return false;
    }

    dma_handle.Instance = hardware->dma_instance;
    dma_handle.Init.Request = hardware->dma_request;
    dma_handle.Init.Direction = DMA_MEMORY_TO_PERIPH;
    dma_handle.Init.PeriphInc = DMA_PINC_DISABLE;
    dma_handle.Init.MemInc = DMA_MINC_ENABLE;
    dma_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    dma_handle.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    dma_handle.Init.Mode = DMA_CIRCULAR;
    dma_handle.Init.Priority = DMA_PRIORITY_HIGH;
    if (HAL_DMA_Init(&dma_handle) != HAL_OK) {
        return false;
    }

    timer_handle.hdma[dma_id_for_channel(hardware->timer_channel)] = &dma_handle;
    dma_handle.Parent = &timer_handle;

    HAL_NVIC_SetPriority((IRQn_Type) hardware->dma_irq, 0U, 0U);
    HAL_NVIC_EnableIRQ((IRQn_Type) hardware->dma_irq);
    return true;
}

static bool start(const uint8_t *data, const uint16_t length) {
    if ((argb_hardware == NULL) || (data == NULL) || (length == 0U)) {
        return false;
    }

    __HAL_TIM_SET_COMPARE(&timer_handle, argb_hardware->timer_channel, 0U);
    return HAL_TIM_PWM_Start_DMA(&timer_handle,
                                 argb_hardware->timer_channel,
                                 (const uint32_t *) data,
                                 length) == HAL_OK;
}

static void stop(void) {
    if (argb_hardware == NULL) {
        return;
    }

    __HAL_TIM_SET_COMPARE(&timer_handle, argb_hardware->timer_channel, 0U);
    (void) HAL_TIM_PWM_Stop_DMA(&timer_handle, argb_hardware->timer_channel);
}

static void dma_irq(void) {
    HAL_DMA_IRQHandler(&dma_handle);
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim) {
    if (htim == &timer_handle) {
        ARGB_Platform_NotifyTransferComplete();
    }
}

void HAL_TIM_PWM_PulseFinishedHalfCpltCallback(TIM_HandleTypeDef *htim) {
    if (htim == &timer_handle) {
        ARGB_Platform_NotifyHalfComplete();
    }
}

const ARGB_Platform ARGB_PLATFORM = {
    .init = init,
    .start = start,
    .stop = stop,
    .dma_irq = dma_irq,
};
