#include "sys/spi.h"
#include "fsm.h"
#include "sys/gpio.h"
#include "stm32h5xx_hal.h"
#include "stm32h5xx_it.h"

typedef enum {
    SPI_FSM_STATE_IDLE = 0,
    SPI_FSM_STATE_DEQUEUE,
    SPI_FSM_STATE_CS_ASSERT,
    SPI_FSM_STATE_READY,
    SPI_FSM_STATE_TX,
    SPI_FSM_STATE_RX,
    SPI_FSM_STATE_CALLBACK,
    SPI_FSM_STATE_CS_DEASSERT
} SPI_FSM_State;

typedef struct {
    SPI_HandleTypeDef hspi;
    DMA_HandleTypeDef hdma;
    FSM fsm;

    SPI_Transaction *queue_head;
    SPI_Transaction *queue_tail;
    SPI_Transaction *active_transaction;
    SPI_Transaction *next_transaction;

    bool active_cs_asserted;
    volatile bool dma_complete;
    volatile bool spi_error;
} spi_t;

static spi_t spi = {0};

static void spi_fsm_idle_service(FSM *fsm);

static void spi_fsm_dequeue_enter(FSM *fsm);

static void spi_fsm_cs_assert_enter(FSM *fsm);

static void spi_fsm_ready_enter(FSM *fsm);

static void spi_fsm_tx_enter(FSM *fsm);

static void spi_fsm_tx_service(FSM *fsm);

static void spi_fsm_rx_enter(FSM *fsm);

static void spi_fsm_rx_service(FSM *fsm);

static void spi_fsm_callback_enter(FSM *fsm);

static void spi_fsm_cs_deassert_enter(FSM *fsm);

static const FSM_State spi_fsm_states[] = {
    [SPI_FSM_STATE_IDLE] = {
        .service = spi_fsm_idle_service,
        .next_mask = FSM_NEXT(SPI_FSM_STATE_DEQUEUE),
    },
    [SPI_FSM_STATE_DEQUEUE] = {
        .enter = spi_fsm_dequeue_enter,
        .next_mask = FSM_NEXT(SPI_FSM_STATE_IDLE) | FSM_NEXT(SPI_FSM_STATE_CS_ASSERT),
    },
    [SPI_FSM_STATE_CS_ASSERT] = {
        .enter = spi_fsm_cs_assert_enter,
        .next_mask = FSM_NEXT(SPI_FSM_STATE_READY) | FSM_NEXT(SPI_FSM_STATE_CALLBACK),
    },
    [SPI_FSM_STATE_READY] = {
        .enter = spi_fsm_ready_enter,
        .next_mask = FSM_NEXT(SPI_FSM_STATE_TX) | FSM_NEXT(SPI_FSM_STATE_RX) | FSM_NEXT(SPI_FSM_STATE_CALLBACK),
    },
    [SPI_FSM_STATE_TX] = {
        .enter = spi_fsm_tx_enter,
        .service = spi_fsm_tx_service,
        .next_mask = FSM_NEXT(SPI_FSM_STATE_RX) | FSM_NEXT(SPI_FSM_STATE_CALLBACK),
    },
    [SPI_FSM_STATE_RX] = {
        .enter = spi_fsm_rx_enter,
        .service = spi_fsm_rx_service,
        .next_mask = FSM_NEXT(SPI_FSM_STATE_CALLBACK),
    },
    [SPI_FSM_STATE_CALLBACK] = {
        .enter = spi_fsm_callback_enter,
        .next_mask = FSM_NEXT(SPI_FSM_STATE_IDLE) | FSM_NEXT(SPI_FSM_STATE_CS_DEASSERT),
    },
    [SPI_FSM_STATE_CS_DEASSERT] = {
        .enter = spi_fsm_cs_deassert_enter,
        .next_mask = FSM_NEXT(SPI_FSM_STATE_IDLE) | FSM_NEXT(SPI_FSM_STATE_CS_ASSERT),
    },
};

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

    spi.hspi.Init.DataSize = data_size;
    spi.hspi.Init.CLKPolarity = tx->ckp ? SPI_POLARITY_HIGH : SPI_POLARITY_LOW;
    spi.hspi.Init.CLKPhase = tx->cke ? SPI_PHASE_2EDGE : SPI_PHASE_1EDGE;
    spi.hspi.Init.BaudRatePrescaler = prescaler;
    spi.hspi.Init.FirstBit = tx->lsb_first ? SPI_FIRSTBIT_LSB : SPI_FIRSTBIT_MSB;

    if (HAL_SPI_Init(&spi.hspi) != HAL_OK) {
        Error_Handler();
    }

    spi.hspi.Instance->UDRDR = 0U;
    return true;
}

static void configure_dma(const SPI_Transaction *tx, const bool receive) {
    if (spi.hdma.Instance != NULL) {
        if (HAL_DMA_DeInit(&spi.hdma) != HAL_OK) {
            Error_Handler();
        }
    }

    spi.hdma.Instance = GPDMA1_Channel1;
    spi.hdma.Init.Request = receive ? GPDMA1_REQUEST_SPI4_RX : GPDMA1_REQUEST_SPI4_TX;
    spi.hdma.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    spi.hdma.Init.Direction = receive ? DMA_PERIPH_TO_MEMORY : DMA_MEMORY_TO_PERIPH;
    spi.hdma.Init.SrcInc = receive ? DMA_SINC_FIXED : DMA_SINC_INCREMENTED;
    spi.hdma.Init.DestInc = receive ? DMA_DINC_INCREMENTED : DMA_DINC_FIXED;
    spi.hdma.Init.SrcDataWidth = tx->bits > 8U ? DMA_SRC_DATAWIDTH_HALFWORD : DMA_SRC_DATAWIDTH_BYTE;
    spi.hdma.Init.DestDataWidth = tx->bits > 8U ? DMA_DEST_DATAWIDTH_HALFWORD : DMA_DEST_DATAWIDTH_BYTE;
    spi.hdma.Init.Priority = DMA_LOW_PRIORITY_HIGH_WEIGHT;
    spi.hdma.Init.SrcBurstLength = 1;
    spi.hdma.Init.DestBurstLength = 1;
    spi.hdma.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0;
    spi.hdma.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    spi.hdma.Init.Mode = DMA_NORMAL;

    if (HAL_DMA_Init(&spi.hdma) != HAL_OK) {
        Error_Handler();
    }

    if (HAL_DMA_ConfigChannelAttributes(&spi.hdma, DMA_CHANNEL_NPRIV) != HAL_OK) {
        Error_Handler();
    }

    if (receive) {
        __HAL_LINKDMA(&spi.hspi, hdmarx, spi.hdma);
    } else {
        __HAL_LINKDMA(&spi.hspi, hdmatx, spi.hdma);
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
    spi.dma_complete = false;
    spi.spi_error = false;
    tx->state = SPI_STATE_TX;

    if (HAL_SPI_Transmit_DMA(&spi.hspi, tx->tx_data, tx->tx_size) != HAL_OK) {
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
    spi.dma_complete = false;
    spi.spi_error = false;
    tx->state = SPI_STATE_RX;

    if (HAL_SPI_Receive_DMA(&spi.hspi, tx->rx_data, tx->rx_size) != HAL_OK) {
        Error_Handler();
    }

    return true;
}

static SPI_Transaction *dequeue_transaction(void) {
    SPI_Transaction *transaction = spi.queue_head;

    if (transaction != NULL) {
        spi.queue_head = transaction->queue_next;
        if (spi.queue_head == NULL) {
            spi.queue_tail = NULL;
        }
        transaction->queue_next = NULL;
    }

    return transaction;
}

static void spi_fsm_idle_service(FSM *fsm) {
    if (spi.queue_head != NULL) {
        FSM_Transition(fsm, SPI_FSM_STATE_DEQUEUE);
    }
}

static void spi_fsm_dequeue_enter(FSM *fsm) {
    spi.active_transaction = dequeue_transaction();
    spi.active_cs_asserted = false;

    if (spi.active_transaction == NULL) {
        FSM_Transition(fsm, SPI_FSM_STATE_IDLE);
        return;
    }

    spi.active_transaction->state = SPI_STATE_IDLE;
    FSM_Transition(fsm, SPI_FSM_STATE_CS_ASSERT);
}

static void spi_fsm_cs_assert_enter(FSM *fsm) {
    if ((spi.active_transaction->cs_port == NULL) || (spi.active_transaction->cs_pin == 0U)) {
        spi.active_transaction->state = SPI_STATE_ERROR;
        FSM_Transition(fsm, SPI_FSM_STATE_CALLBACK);
        return;
    }

    HAL_GPIO_WritePin(spi.active_transaction->cs_port, spi.active_transaction->cs_pin, GPIO_PIN_RESET);
    spi.active_cs_asserted = true;
    FSM_Transition(fsm, SPI_FSM_STATE_READY);
}

static void spi_fsm_ready_enter(FSM *fsm) {
    switch (spi.active_transaction->operation) {
        case SPI_OPERATION_WRITE:
        case SPI_OPERATION_WRITE_THEN_READ:
            FSM_Transition(fsm, SPI_FSM_STATE_TX);
            break;
        case SPI_OPERATION_READ:
            FSM_Transition(fsm, SPI_FSM_STATE_RX);
            break;
        default:
            spi.active_transaction->state = SPI_STATE_ERROR;
            FSM_Transition(fsm, SPI_FSM_STATE_CALLBACK);
            break;
    }
}

static void spi_fsm_tx_enter(FSM *fsm) {
    if (!start_tx_phase(spi.active_transaction)) {
        spi.active_transaction->state = SPI_STATE_ERROR;
        FSM_Transition(fsm, SPI_FSM_STATE_CALLBACK);
    }
}

static void spi_fsm_tx_service(FSM *fsm) {
    if (!spi.dma_complete) {
        return;
    }

    spi.dma_complete = false;

    if ((spi.active_transaction->state == SPI_STATE_TX) &&
        (spi.active_transaction->operation == SPI_OPERATION_WRITE_THEN_READ)) {
        FSM_Transition(fsm, SPI_FSM_STATE_RX);
    } else {
        FSM_Transition(fsm, SPI_FSM_STATE_CALLBACK);
    }
}

static void spi_fsm_rx_enter(FSM *fsm) {
    if (!start_rx_phase(spi.active_transaction)) {
        spi.active_transaction->state = SPI_STATE_ERROR;
        FSM_Transition(fsm, SPI_FSM_STATE_CALLBACK);
    }
}

static void spi_fsm_rx_service(FSM *fsm) {
    if (!spi.dma_complete) {
        return;
    }

    spi.dma_complete = false;
    FSM_Transition(fsm, SPI_FSM_STATE_CALLBACK);
}

static void spi_fsm_callback_enter(FSM *fsm) {
    spi.next_transaction = NULL;

    if (spi.active_transaction == NULL) {
        FSM_Transition(fsm, SPI_FSM_STATE_IDLE);
        return;
    }

    if (spi.active_transaction->state != SPI_STATE_ERROR) {
        spi.active_transaction->state = SPI_STATE_COMPLETE;
    }

    if (spi.active_transaction->callback != NULL) {
        spi.next_transaction = spi.active_transaction->callback(spi.active_transaction);
    }

    if (spi.next_transaction == NULL) {
        spi.next_transaction = dequeue_transaction();
    }

    FSM_Transition(fsm, SPI_FSM_STATE_CS_DEASSERT);
}

static void spi_fsm_cs_deassert_enter(FSM *fsm) {
    if ((spi.active_transaction != NULL) && spi.active_cs_asserted &&
        (!spi.active_transaction->cs_hold || (spi.next_transaction == NULL) ||
         (spi.active_transaction->cs_port != spi.next_transaction->cs_port) ||
         (spi.active_transaction->cs_pin != spi.next_transaction->cs_pin))) {
        HAL_GPIO_WritePin(spi.active_transaction->cs_port, spi.active_transaction->cs_pin, GPIO_PIN_SET);
    }

    spi.active_transaction = spi.next_transaction;
    spi.next_transaction = NULL;
    spi.active_cs_asserted = false;

    if (spi.active_transaction != NULL) {
        spi.active_transaction->queue_next = NULL;
        spi.active_transaction->state = SPI_STATE_IDLE;
        FSM_Transition(fsm, SPI_FSM_STATE_CS_ASSERT);
    } else {
        FSM_Transition(fsm, SPI_FSM_STATE_IDLE);
    }
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
    spi.hspi.Instance = SPI4;
    spi.hspi.Init.Mode = SPI_MODE_MASTER;
    spi.hspi.Init.Direction = SPI_DIRECTION_2LINES;
    spi.hspi.Init.DataSize = SPI_DATASIZE_8BIT;
    spi.hspi.Init.CLKPolarity = SPI_POLARITY_LOW;
    spi.hspi.Init.CLKPhase = SPI_PHASE_1EDGE;
    spi.hspi.Init.NSS = SPI_NSS_SOFT;
    spi.hspi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
    spi.hspi.Init.FirstBit = SPI_FIRSTBIT_MSB;
    spi.hspi.Init.TIMode = SPI_TIMODE_DISABLE;
    spi.hspi.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    spi.hspi.Init.CRCPolynomial = 0x7;
    spi.hspi.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
    spi.hspi.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
    spi.hspi.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
    spi.hspi.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
    spi.hspi.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
    spi.hspi.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
    spi.hspi.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
    spi.hspi.Init.IOSwap = SPI_IO_SWAP_DISABLE;
    spi.hspi.Init.ReadyMasterManagement = SPI_RDY_MASTER_MANAGEMENT_INTERNALLY;
    spi.hspi.Init.ReadyPolarity = SPI_RDY_POLARITY_HIGH;

    if (HAL_SPI_Init(&spi.hspi) != HAL_OK) {
        Error_Handler();
    }

    if (!FSM_Init(&spi.fsm, spi_fsm_states, SPI_FSM_STATE_IDLE, NULL)) {
        Error_Handler();
    }
}

void SPI_Service(void) {
    if (spi.spi_error) {
        spi.spi_error = false;
        if (spi.active_transaction != NULL) {
            spi.active_transaction->state = SPI_STATE_ERROR;
            spi.dma_complete = true;
        }
    }

    FSM_Service(&spi.fsm);
}

void SPI_Queue(SPI_Transaction *tx) {
    if (tx == NULL) {
        return;
    }

    tx->queue_next = NULL;
    tx->state = SPI_STATE_IDLE;

    if (spi.queue_tail == NULL) {
        spi.queue_head = tx;
        spi.queue_tail = tx;
    } else {
        spi.queue_tail->queue_next = tx;
        spi.queue_tail = tx;
    }
}

void GPDMA1_Channel1_IRQHandler(void) {
    HAL_DMA_IRQHandler(&spi.hdma);
}

void SPI4_IRQHandler(void) {
    HAL_SPI_IRQHandler(&spi.hspi);
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
    if (hspi->Instance == SPI4) {
        spi.dma_complete = true;
    }
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi) {
    if (hspi->Instance == SPI4) {
        spi.dma_complete = true;
    }
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi) {
    if (hspi->Instance == SPI4) {
        spi.spi_error = true;
    }
}
