#include "argb_platform.h"

#include "stm32h5xx_hal.h"

static const ARGB_Hardware *argb_hardware;
static TIM_HandleTypeDef timer_handle;
static DMA_HandleTypeDef dma_handle;
static DMA_NodeConfTypeDef dma_node_config;
static DMA_NodeTypeDef dma_node;
static DMA_QListTypeDef dma_queue;

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

static bool dma_init(const ARGB_Hardware *hardware) {
    dma_node_config.NodeType = DMA_GPDMA_LINEAR_NODE;
    dma_node_config.Init.Request = hardware->dma_request;
    dma_node_config.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    dma_node_config.Init.Direction = DMA_MEMORY_TO_PERIPH;
    dma_node_config.Init.SrcInc = DMA_SINC_INCREMENTED;
    dma_node_config.Init.DestInc = DMA_DINC_FIXED;
    dma_node_config.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    dma_node_config.Init.DestDataWidth = DMA_DEST_DATAWIDTH_WORD;
    dma_node_config.Init.SrcBurstLength = 1U;
    dma_node_config.Init.DestBurstLength = 1U;
    dma_node_config.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0;
    dma_node_config.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    dma_node_config.Init.Mode = DMA_NORMAL;
    dma_node_config.TriggerConfig.TriggerPolarity = DMA_TRIG_POLARITY_MASKED;
    dma_node_config.DataHandlingConfig.DataExchange = DMA_EXCHANGE_NONE;
    dma_node_config.DataHandlingConfig.DataAlignment = DMA_DATA_RIGHTALIGN_ZEROPADDED;

    if ((HAL_DMAEx_List_BuildNode(&dma_node_config, &dma_node) != HAL_OK) ||
        (HAL_DMAEx_List_InsertNode(&dma_queue, NULL, &dma_node) != HAL_OK) ||
        (HAL_DMAEx_List_SetCircularMode(&dma_queue) != HAL_OK)) {
        return false;
    }

    dma_handle.Instance = hardware->dma_instance;
    dma_handle.InitLinkedList.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
    dma_handle.InitLinkedList.LinkStepMode = DMA_LSM_FULL_EXECUTION;
    dma_handle.InitLinkedList.LinkAllocatedPort = DMA_LINK_ALLOCATED_PORT0;
    dma_handle.InitLinkedList.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    dma_handle.InitLinkedList.LinkedListMode = DMA_LINKEDLIST_CIRCULAR;
    if ((HAL_DMAEx_List_Init(&dma_handle) != HAL_OK) ||
        (HAL_DMAEx_List_LinkQ(&dma_handle, &dma_queue) != HAL_OK) ||
        (HAL_DMA_ConfigChannelAttributes(&dma_handle, DMA_CHANNEL_NPRIV) != HAL_OK)) {
        return false;
    }

    timer_handle.hdma[dma_id_for_channel(hardware->timer_channel)] = &dma_handle;
    dma_handle.Parent = &timer_handle;
    return true;
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
    output_config.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&timer_handle, &output_config, hardware->timer_channel) != HAL_OK) {
        return false;
    }

    if (!dma_init(hardware)) {
        return false;
    }

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
