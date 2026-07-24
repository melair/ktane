#include "spi.h"
#include "gpio.h"
#include "stm32h5xx_hal.h"
#include "stm32h5xx_it.h"

static SPI_HandleTypeDef hspi4 = {0};
static DMA_HandleTypeDef hdma_gpdma1_channel1 = {0};

static SPI_Transaction *queue_head = NULL;
static SPI_Transaction *queue_tail = NULL;
static SPI_Transaction *active_transaction = NULL;
static bool active_cs_asserted = false;
static volatile bool dma_complete = false;
static volatile bool spi_error = false;

static bool spi_data_size(const uint8_t bits, uint32_t *data_size) {
    if ((bits < 4U) || (bits > 16U)) {
        return false;
    }

    *data_size = (uint32_t) (bits - 1U);
    return true;
}

static bool spi_baud_prescaler(const SPI_Baud baud, uint32_t *prescaler) {
    switch (baud) {
        case SPI_BAUD_8MHZ:
            *prescaler = SPI_BAUDRATEPRESCALER_4;
            return true;
        case SPI_BAUD_4MHZ:
            *prescaler = SPI_BAUDRATEPRESCALER_8;
            return true;
        case SPI_BAUD_2MHZ:
            *prescaler = SPI_BAUDRATEPRESCALER_16;
            return true;
        case SPI_BAUD_1MHZ:
            *prescaler = SPI_BAUDRATEPRESCALER_32;
            return true;
        case SPI_BAUD_500KHZ:
            *prescaler = SPI_BAUDRATEPRESCALER_64;
            return true;
        case SPI_BAUD_250KHZ:
            *prescaler = SPI_BAUDRATEPRESCALER_128;
            return true;
        case SPI_BAUD_125KHZ:
            *prescaler = SPI_BAUDRATEPRESCALER_256;
            return true;
        default:
            return false;
    }
}

static bool configure_spi(const SPI_Transaction *tx) {
    uint32_t data_size = 0;
    uint32_t prescaler = 0;

    if (!spi_data_size(tx->bits, &data_size) || !spi_baud_prescaler(tx->baud, &prescaler)) {
        return false;
    }

    hspi4.Init.DataSize = data_size;
    hspi4.Init.CLKPolarity = tx->ckp ? SPI_POLARITY_HIGH : SPI_POLARITY_LOW;
    hspi4.Init.CLKPhase = tx->cke ? SPI_PHASE_2EDGE : SPI_PHASE_1EDGE;
    hspi4.Init.BaudRatePrescaler = prescaler;
    hspi4.Init.FirstBit = tx->lsb_first ? SPI_FIRSTBIT_LSB : SPI_FIRSTBIT_MSB;

    if (HAL_SPI_Init(&hspi4) != HAL_OK) {
        Error_Handler();
    }

    hspi4.Instance->UDRDR = 0U;
    return true;
}

static void configure_dma(const SPI_Transaction *tx, const bool receive) {
    if (hdma_gpdma1_channel1.Instance != NULL) {
        if (HAL_DMA_DeInit(&hdma_gpdma1_channel1) != HAL_OK) {
            Error_Handler();
        }
    }

    hdma_gpdma1_channel1.Instance = GPDMA1_Channel1;
    hdma_gpdma1_channel1.Init.Request = receive ? GPDMA1_REQUEST_SPI4_RX : GPDMA1_REQUEST_SPI4_TX;
    hdma_gpdma1_channel1.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    hdma_gpdma1_channel1.Init.Direction = receive ? DMA_PERIPH_TO_MEMORY : DMA_MEMORY_TO_PERIPH;
    hdma_gpdma1_channel1.Init.SrcInc = receive ? DMA_SINC_FIXED : DMA_SINC_INCREMENTED;
    hdma_gpdma1_channel1.Init.DestInc = receive ? DMA_DINC_INCREMENTED : DMA_DINC_FIXED;
    hdma_gpdma1_channel1.Init.SrcDataWidth = tx->bits > 8U ? DMA_SRC_DATAWIDTH_HALFWORD : DMA_SRC_DATAWIDTH_BYTE;
    hdma_gpdma1_channel1.Init.DestDataWidth = tx->bits > 8U ? DMA_DEST_DATAWIDTH_HALFWORD : DMA_DEST_DATAWIDTH_BYTE;
    hdma_gpdma1_channel1.Init.Priority = DMA_LOW_PRIORITY_HIGH_WEIGHT;
    hdma_gpdma1_channel1.Init.SrcBurstLength = 1;
    hdma_gpdma1_channel1.Init.DestBurstLength = 1;
    hdma_gpdma1_channel1.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0;
    hdma_gpdma1_channel1.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    hdma_gpdma1_channel1.Init.Mode = DMA_NORMAL;

    if (HAL_DMA_Init(&hdma_gpdma1_channel1) != HAL_OK) {
        Error_Handler();
    }

    if (HAL_DMA_ConfigChannelAttributes(&hdma_gpdma1_channel1, DMA_CHANNEL_NPRIV) != HAL_OK) {
        Error_Handler();
    }

    if (receive) {
        __HAL_LINKDMA(&hspi4, hdmarx, hdma_gpdma1_channel1);
    } else {
        __HAL_LINKDMA(&hspi4, hdmatx, hdma_gpdma1_channel1);
    }
}

static bool start_tx_phase(SPI_Transaction *tx) {
    if ((tx->tx_data == NULL) || (tx->tx_size == 0U)) {
        return false;
    }

    if (!configure_spi(tx)) {
        return false;
    }

    configure_dma(tx, false);
    dma_complete = false;
    spi_error = false;
    tx->state = SPI_STATE_TX;

    if (HAL_SPI_Transmit_DMA(&hspi4, tx->tx_data, tx->tx_size) != HAL_OK) {
        Error_Handler();
    }

    return true;
}

static bool start_rx_phase(SPI_Transaction *tx) {
    if ((tx->rx_data == NULL) || (tx->rx_size == 0U)) {
        return false;
    }

    if (!configure_spi(tx)) {
        return false;
    }

    configure_dma(tx, true);
    dma_complete = false;
    spi_error = false;
    tx->state = SPI_STATE_RX;

    if (HAL_SPI_Receive_DMA(&hspi4, tx->rx_data, tx->rx_size) != HAL_OK) {
        Error_Handler();
    }

    return true;
}

static bool start_transaction(SPI_Transaction *tx) {
    active_transaction = tx;
    active_transaction->queue_next = NULL;
    active_transaction->state = SPI_STATE_IDLE;
    active_cs_asserted = false;

    if ((tx->cs_port == NULL) || (tx->cs_pin == 0U)) {
        active_transaction->state = SPI_STATE_ERROR;
        return false;
    }

    HAL_GPIO_WritePin(tx->cs_port, tx->cs_pin, GPIO_PIN_RESET);
    active_cs_asserted = true;

    switch (tx->operation) {
        case SPI_OPERATION_WRITE:
            return start_tx_phase(tx);
        case SPI_OPERATION_WRITE_THEN_READ:
            return start_tx_phase(tx);
        case SPI_OPERATION_READ:
            return start_rx_phase(tx);
        default:
            active_transaction->state = SPI_STATE_ERROR;
            return false;
    }
}

static SPI_Transaction *dequeue_transaction(void) {
    SPI_Transaction *transaction = queue_head;

    if (transaction != NULL) {
        queue_head = transaction->queue_next;
        if (queue_head == NULL) {
            queue_tail = NULL;
        }
        transaction->queue_next = NULL;
    }

    return transaction;
}

/**
  * @brief SPI MSP Initialization
  * This function configures the hardware resources used in this example
  * @param hspi: SPI handle pointer
  * @retval None
  */
void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (hspi->Instance == SPI4) {
        /* Initializes the peripherals clock */
        __HAL_RCC_SPI4_CONFIG(RCC_SPI4CLKSOURCE_HSI);

        /* Peripheral clock enable */
        __HAL_RCC_SPI4_CLK_ENABLE();

        HAL_NVIC_SetPriority(GPDMA1_Channel1_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(GPDMA1_Channel1_IRQn);

        HAL_NVIC_SetPriority(SPI4_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(SPI4_IRQn);

        /* SPI4 GPIO Configuration
         * PE12     ------> SPI4_SCK
         * PE13     ------> SPI4_MISO
         * PE14     ------> SPI4_MOSI
         */
        GPIO_InitStruct.Pin = SPI_SCK_Pin | SPI_MOSI_Pin | SPI_MISO_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF5_SPI4;
        HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
    }
}

/**
  * @brief SPI MSP De-Initialization
  * This function freeze the hardware resources used in this example
  * @param hspi: SPI handle pointer
  * @retval None
  */
void HAL_SPI_MspDeInit(SPI_HandleTypeDef *hspi) {
    if (hspi->Instance == SPI4) {
        /* Peripheral clock disable */
        __HAL_RCC_SPI4_CLK_DISABLE();

        /**SPI4 GPIO Configuration
        PE12     ------> SPI4_SCK
        PE13     ------> SPI4_MISO
        PE14     ------> SPI4_MOSI
        */
        HAL_GPIO_DeInit(GPIOE, SPI_SCK_Pin | SPI_MOSI_Pin | SPI_MISO_Pin);

        HAL_NVIC_DisableIRQ(GPDMA1_Channel1_IRQn);
        HAL_NVIC_DisableIRQ(SPI4_IRQn);
    }
}

void SPI_Init(void) {
    /* Initialise SPI4 peripheral at 1MHz. */
    hspi4.Instance = SPI4;
    hspi4.Init.Mode = SPI_MODE_MASTER;
    hspi4.Init.Direction = SPI_DIRECTION_2LINES;
    hspi4.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi4.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi4.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi4.Init.NSS = SPI_NSS_SOFT;
    hspi4.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
    hspi4.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi4.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi4.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi4.Init.CRCPolynomial = 0x7;
    hspi4.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
    hspi4.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
    hspi4.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
    hspi4.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
    hspi4.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
    hspi4.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
    hspi4.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
    hspi4.Init.IOSwap = SPI_IO_SWAP_DISABLE;
    hspi4.Init.ReadyMasterManagement = SPI_RDY_MASTER_MANAGEMENT_INTERNALLY;
    hspi4.Init.ReadyPolarity = SPI_RDY_POLARITY_HIGH;

    if (HAL_SPI_Init(&hspi4) != HAL_OK) {
        Error_Handler();
    }
}

void SPI_Service(void) {
    if (spi_error) {
        spi_error = false;
        if (active_transaction != NULL) {
            active_transaction->state = SPI_STATE_ERROR;
            dma_complete = true;
        }
    }

    if (active_transaction == NULL) {
        SPI_Transaction *next = dequeue_transaction();
        if (next != NULL) {
            if (!start_transaction(next)) {
                next->state = SPI_STATE_ERROR;
                dma_complete = true;
            }
        }
        return;
    }

    if (!dma_complete) {
        return;
    }
    dma_complete = false;

    if ((active_transaction->state == SPI_STATE_TX) &&
        (active_transaction->operation == SPI_OPERATION_WRITE_THEN_READ)) {
        if (start_rx_phase(active_transaction)) {
            return;
        }

        active_transaction->state = SPI_STATE_ERROR;
    }

    if (active_transaction->state != SPI_STATE_ERROR) {
        active_transaction->state = SPI_STATE_COMPLETE;
    }

    SPI_Transaction *completed_transaction = active_transaction;
    GPIO_TypeDef *completed_cs_port = completed_transaction->cs_port;
    uint32_t completed_cs_pin = completed_transaction->cs_pin;
    bool completed_cs_hold = completed_transaction->cs_hold;
    SPI_Transaction *callback_transaction = NULL;

    if (completed_transaction->callback != NULL) {
        callback_transaction = completed_transaction->callback(completed_transaction);
    }

    SPI_Transaction *next_transaction = callback_transaction;
    if (next_transaction == NULL) {
        next_transaction = dequeue_transaction();
    }

    if (active_cs_asserted && (!completed_cs_hold || (next_transaction == NULL) ||
        (completed_cs_port != next_transaction->cs_port) || (completed_cs_pin != next_transaction->cs_pin))) {
        HAL_GPIO_WritePin(completed_cs_port, completed_cs_pin, GPIO_PIN_SET);
    }

    active_transaction = NULL;
    active_cs_asserted = false;

    if (next_transaction != NULL) {
        if (!start_transaction(next_transaction)) {
            next_transaction->state = SPI_STATE_ERROR;
            dma_complete = true;
        }
    }
}

void SPI_Queue(SPI_Transaction *tx) {
    if (tx == NULL) {
        return;
    }

    tx->queue_next = NULL;
    tx->state = SPI_STATE_IDLE;

    if (queue_tail == NULL) {
        queue_head = tx;
        queue_tail = tx;
    } else {
        queue_tail->queue_next = tx;
        queue_tail = tx;
    }
}

void GPDMA1_Channel1_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdma_gpdma1_channel1);
}

void SPI4_IRQHandler(void) {
    HAL_SPI_IRQHandler(&hspi4);
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
    if (hspi->Instance == SPI4) {
        dma_complete = true;
    }
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi) {
    if (hspi->Instance == SPI4) {
        dma_complete = true;
    }
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi) {
    if (hspi->Instance == SPI4) {
        spi_error = true;
    }
}
