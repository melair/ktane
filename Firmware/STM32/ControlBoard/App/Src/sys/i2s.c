#include "../../Inc/sys/i2s.h"
#include "sys/gpio.h"
#include "stm32h5xx_hal_i2s.h"
#include "stm32h5xx_it.h"

#define I2S_WS_Pin GPIO_B0_Pin
#define I2S_WS_Port GPIO_B0_Port
#define I2S_CK_Pin GPIO_B1_Pin
#define I2S_CK_Port GPIO_B1_Port
#define I2S_SDI_Pin GPIO_B2_Pin
#define I2S_SDI_Port GPIO_B2_Port
#define I2S_SDO_Pin GPIO_B3_Pin
#define I2S_SDO_Port GPIO_B3_Port
#define I2S_MCK_Pin GPIO_B4_Pin
#define I2S_MCK_Port GPIO_B4_Port

static I2S_HandleTypeDef i2s = {0};
static DMA_NodeConfTypeDef i2s_dma_node_config = {0};
static DMA_NodeTypeDef i2s_dma_node = {0};
static DMA_QListTypeDef i2s_dma_list = {0};
static DMA_HandleTypeDef i2s_dma = {0};

static const int16_t i2s_sine_1khz_minus_3db[] = {
    0, 3028, 6004, 8877, 11599, 14122, 16403, 18404,
    20089, 21431, 22407, 22999, 23197, 22999, 22407, 21431,
    20089, 18404, 16403, 14122, 11599, 8877, 6004, 3028,
    0, -3028, -6004, -8877, -11599, -14122, -16403, -18404,
    -20089, -21431, -22407, -22999, -23197, -22999, -22407, -21431,
    -20089, -18404, -16403, -14122, -11599, -8877, -6004, -3028,
};

void I2S_Init(AudioData *audio) {
    GPIO_InitTypeDef gpio_init = {0};

    __HAL_RCC_SPI1_CONFIG(RCC_SPI1CLKSOURCE_PLL3P);
    __HAL_RCC_SPI1_CLK_ENABLE();

    HAL_GPIO_WritePin(I2S_WS_Port, I2S_WS_Pin | I2S_CK_Pin | I2S_SDO_Pin, GPIO_PIN_RESET);

    gpio_init.Pin = I2S_WS_Pin | I2S_CK_Pin | I2S_SDO_Pin;
    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    gpio_init.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(I2S_WS_Port, &gpio_init);

    i2s.Instance = SPI1;
    i2s.Init.Mode = I2S_MODE_MASTER_TX;
    i2s.Init.Standard = I2S_STANDARD_PHILIPS;
    i2s.Init.DataFormat = I2S_DATAFORMAT_16B_EXTENDED;
    i2s.Init.MCLKOutput = I2S_MCLKOUTPUT_DISABLE;
    i2s.Init.AudioFreq = I2S_AUDIOFREQ_48K;
    i2s.Init.CPOL = I2S_CPOL_LOW;
    i2s.Init.FirstBit = I2S_FIRSTBIT_MSB;
    i2s.Init.WSInversion = I2S_WS_INVERSION_DISABLE;
    i2s.Init.Data24BitAlignment = I2S_DATA_24BIT_ALIGNMENT_RIGHT;
    i2s.Init.MasterKeepIOState = I2S_MASTER_KEEP_IO_STATE_ENABLE;

    if (HAL_I2S_Init(&i2s) != HAL_OK) {
        Error_Handler();
    }

    HAL_NVIC_SetPriority(GPDMA1_Channel3_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel3_IRQn);

    i2s_dma_node_config.NodeType = DMA_GPDMA_LINEAR_NODE;
    i2s_dma_node_config.Init.Request = GPDMA1_REQUEST_SPI1_TX;
    i2s_dma_node_config.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    i2s_dma_node_config.Init.Direction = DMA_MEMORY_TO_PERIPH;
    i2s_dma_node_config.Init.SrcInc = DMA_SINC_INCREMENTED;
    i2s_dma_node_config.Init.DestInc = DMA_DINC_FIXED;
    i2s_dma_node_config.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_HALFWORD;
    i2s_dma_node_config.Init.DestDataWidth = DMA_DEST_DATAWIDTH_HALFWORD;
    i2s_dma_node_config.Init.SrcBurstLength = 1;
    i2s_dma_node_config.Init.DestBurstLength = 1;
    i2s_dma_node_config.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0;
    i2s_dma_node_config.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    i2s_dma_node_config.Init.Mode = DMA_NORMAL;
    i2s_dma_node_config.TriggerConfig.TriggerPolarity = DMA_TRIG_POLARITY_MASKED;
    i2s_dma_node_config.DataHandlingConfig.DataExchange = DMA_EXCHANGE_NONE;
    i2s_dma_node_config.DataHandlingConfig.DataAlignment = DMA_DATA_RIGHTALIGN_ZEROPADDED;

    if (HAL_DMAEx_List_BuildNode(&i2s_dma_node_config, &i2s_dma_node) != HAL_OK) {
        Error_Handler();
    }

    if (HAL_DMAEx_List_InsertNode(&i2s_dma_list, NULL, &i2s_dma_node) != HAL_OK) {
        Error_Handler();
    }

    if (HAL_DMAEx_List_SetCircularMode(&i2s_dma_list) != HAL_OK) {
        Error_Handler();
    }

    i2s_dma.Instance = GPDMA1_Channel3;
    i2s_dma.Init.Mode = DMA_LINKEDLIST_CIRCULAR;
    i2s_dma.InitLinkedList.Priority = DMA_LOW_PRIORITY_HIGH_WEIGHT;
    i2s_dma.InitLinkedList.LinkStepMode = DMA_LSM_FULL_EXECUTION;
    i2s_dma.InitLinkedList.LinkAllocatedPort = DMA_LINK_ALLOCATED_PORT0;
    i2s_dma.InitLinkedList.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    i2s_dma.InitLinkedList.LinkedListMode = DMA_LINKEDLIST_CIRCULAR;

    if (HAL_DMAEx_List_Init(&i2s_dma) != HAL_OK) {
        Error_Handler();
    }

    if (HAL_DMAEx_List_LinkQ(&i2s_dma, &i2s_dma_list) != HAL_OK) {
        Error_Handler();
    }

    __HAL_LINKDMA(&i2s, hdmatx, i2s_dma);

    if (HAL_DMA_ConfigChannelAttributes(&i2s_dma, DMA_CHANNEL_NPRIV) != HAL_OK) {
        Error_Handler();
    }

    if (HAL_I2S_Transmit_DMA(&i2s, (const uint16_t *) audio->buffer,
                             audio->buffer_size) != HAL_OK) {
        Error_Handler();
    }
}

void I2S_Service(AudioData *audio) {
}

void I2S_Fill_Sine(AudioData *audio) {
    for (uint16_t sample = 0; sample + 1u < audio->buffer_size; sample += I2S_AUDIO_CHANNEL_COUNT) {
        const int16_t value = i2s_sine_1khz_minus_3db[
            (sample / I2S_AUDIO_CHANNEL_COUNT) % 48u];
        audio->buffer[sample] = value;
        audio->buffer[sample + 1u] = value;
    }
}

void GPDMA1_Channel3_IRQHandler(void) {
    HAL_DMA_IRQHandler(&i2s_dma);
}

void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s) {
}

void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s) {
}
