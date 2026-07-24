#include "i2c.h"
#include "fsm.h"
#include "gpio.h"
#include "stm32h5xx_hal.h"
#include "stm32h5xx_it.h"

typedef enum {
    I2C_FSM_STATE_IDLE = 0,
    I2C_FSM_STATE_DEQUEUE,
    I2C_FSM_STATE_CONFIGURE,
    I2C_FSM_STATE_WRITE,
    I2C_FSM_STATE_READ,
    I2C_FSM_STATE_CALLBACK
} I2C_FSM_State;

typedef struct {
    I2C_HandleTypeDef hi2c;
    DMA_HandleTypeDef hdma;
    FSM fsm;

    I2C_Transaction *queue_head;
    I2C_Transaction *queue_tail;
    I2C_Transaction *current;

    volatile bool transfer_done;
    volatile bool transfer_error;
    volatile I2C_Status error_status;
} i2c_t;

static i2c_t i2c = {0};

static void i2c_fsm_idle_service(FSM *fsm);
static void i2c_fsm_dequeue_enter(FSM *fsm);
static void i2c_fsm_configure_enter(FSM *fsm);
static void i2c_fsm_write_enter(FSM *fsm);
static void i2c_fsm_write_service(FSM *fsm);
static void i2c_fsm_read_enter(FSM *fsm);
static void i2c_fsm_read_service(FSM *fsm);
static void i2c_fsm_callback_enter(FSM *fsm);

static const FSM_State i2c_fsm_states[] = {
    [I2C_FSM_STATE_IDLE] = {
        .service = i2c_fsm_idle_service,
        .next_mask = FSM_NEXT(I2C_FSM_STATE_DEQUEUE),
    },
    [I2C_FSM_STATE_DEQUEUE] = {
        .enter = i2c_fsm_dequeue_enter,
        .next_mask = FSM_NEXT(I2C_FSM_STATE_CONFIGURE),
    },
    [I2C_FSM_STATE_CONFIGURE] = {
        .enter = i2c_fsm_configure_enter,
        .next_mask = FSM_NEXT(I2C_FSM_STATE_WRITE) | FSM_NEXT(I2C_FSM_STATE_READ) | FSM_NEXT(I2C_FSM_STATE_CALLBACK),
    },
    [I2C_FSM_STATE_WRITE] = {
        .enter = i2c_fsm_write_enter,
        .service = i2c_fsm_write_service,
        .next_mask = FSM_NEXT(I2C_FSM_STATE_READ) | FSM_NEXT(I2C_FSM_STATE_CALLBACK),
    },
    [I2C_FSM_STATE_READ] = {
        .enter = i2c_fsm_read_enter,
        .service = i2c_fsm_read_service,
        .next_mask = FSM_NEXT(I2C_FSM_STATE_CALLBACK),
    },
    [I2C_FSM_STATE_CALLBACK] = {
        .enter = i2c_fsm_callback_enter,
        .next_mask = FSM_NEXT(I2C_FSM_STATE_IDLE) | FSM_NEXT(I2C_FSM_STATE_CONFIGURE),
    },
};

static uint16_t i2c_device_address(const I2C_Transaction *tx) {
    return (uint16_t) tx->address << 1U;
}

static I2C_Status i2c_hal_error_status(void) {
    const uint32_t error = HAL_I2C_GetError(&i2c.hi2c);

    if ((error & HAL_I2C_ERROR_AF) != 0U) {
        return I2C_STATUS_ERROR_NACK;
    }

    if ((error & HAL_I2C_ERROR_TIMEOUT) != 0U) {
        return I2C_STATUS_ERROR_BTO;
    }

    return I2C_STATUS_ERROR_UNKNOWN;
}

static void configure_dma(const bool receive) {
    if (i2c.hdma.Instance != NULL) {
        if (HAL_DMA_DeInit(&i2c.hdma) != HAL_OK) {
            Error_Handler();
        }
    }

    i2c.hdma.Instance = GPDMA1_Channel2;
    i2c.hdma.Init.Request = receive ? GPDMA1_REQUEST_I2C2_RX : GPDMA1_REQUEST_I2C2_TX;
    i2c.hdma.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    i2c.hdma.Init.Direction = receive ? DMA_PERIPH_TO_MEMORY : DMA_MEMORY_TO_PERIPH;
    i2c.hdma.Init.SrcInc = receive ? DMA_SINC_FIXED : DMA_SINC_INCREMENTED;
    i2c.hdma.Init.DestInc = receive ? DMA_DINC_INCREMENTED : DMA_DINC_FIXED;
    i2c.hdma.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    i2c.hdma.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    i2c.hdma.Init.Priority = DMA_LOW_PRIORITY_HIGH_WEIGHT;
    i2c.hdma.Init.SrcBurstLength = 1;
    i2c.hdma.Init.DestBurstLength = 1;
    i2c.hdma.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0;
    i2c.hdma.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    i2c.hdma.Init.Mode = DMA_NORMAL;

    if (HAL_DMA_Init(&i2c.hdma) != HAL_OK) {
        Error_Handler();
    }

    if (HAL_DMA_ConfigChannelAttributes(&i2c.hdma, DMA_CHANNEL_NPRIV) != HAL_OK) {
        Error_Handler();
    }

    if (receive) {
        __HAL_LINKDMA(&i2c.hi2c, hdmarx, i2c.hdma);
    } else {
        __HAL_LINKDMA(&i2c.hi2c, hdmatx, i2c.hdma);
    }
}

static bool start_write_phase(I2C_Transaction *tx) {
    if ((tx->tx_data == NULL) || (tx->tx_size == 0U)) {
        return false;
    }

    configure_dma(false);
    i2c.transfer_done = false;
    i2c.transfer_error = false;

    if (tx->operation == I2C_OPERATION_WRITE_RESTART_READ) {
        return HAL_I2C_Master_Seq_Transmit_DMA(&i2c.hi2c, i2c_device_address(tx),
                                               tx->tx_data, tx->tx_size, I2C_FIRST_FRAME) == HAL_OK;
    }

    return HAL_I2C_Master_Transmit_DMA(&i2c.hi2c, i2c_device_address(tx),
                                       tx->tx_data, tx->tx_size) == HAL_OK;
}

static bool start_read_phase(I2C_Transaction *tx) {
    if ((tx->rx_data == NULL) || (tx->rx_size == 0U)) {
        return false;
    }

    configure_dma(true);
    i2c.transfer_done = false;
    i2c.transfer_error = false;

    if (tx->operation == I2C_OPERATION_WRITE_RESTART_READ) {
        return HAL_I2C_Master_Seq_Receive_DMA(&i2c.hi2c, i2c_device_address(tx),
                                              tx->rx_data, tx->rx_size, I2C_LAST_FRAME) == HAL_OK;
    }

    return HAL_I2C_Master_Receive_DMA(&i2c.hi2c, i2c_device_address(tx),
                                      tx->rx_data, tx->rx_size) == HAL_OK;
}

static I2C_Transaction *dequeue_transaction(void) {
    I2C_Transaction *transaction = i2c.queue_head;

    if (transaction != NULL) {
        i2c.queue_head = transaction->queue_next;
        if (i2c.queue_head == NULL) {
            i2c.queue_tail = NULL;
        }
        transaction->queue_next = NULL;
    }

    return transaction;
}

static void i2c_fsm_idle_service(FSM *fsm) {
    if (i2c.queue_head != NULL) {
        FSM_Transition(fsm, I2C_FSM_STATE_DEQUEUE);
    }
}

static void i2c_fsm_dequeue_enter(FSM *fsm) {
    i2c.current = dequeue_transaction();
    i2c.current->status = I2C_STATUS_SUCCESS;
    FSM_Transition(fsm, I2C_FSM_STATE_CONFIGURE);
}

static void i2c_fsm_configure_enter(FSM *fsm) {
    switch (i2c.current->operation) {
        case I2C_OPERATION_WRITE:
        case I2C_OPERATION_WRITE_STOP_READ:
        case I2C_OPERATION_WRITE_RESTART_READ:
            FSM_Transition(fsm, I2C_FSM_STATE_WRITE);
            break;
        case I2C_OPERATION_READ:
            FSM_Transition(fsm, I2C_FSM_STATE_READ);
            break;
        default:
            i2c.current->status = I2C_STATUS_ERROR_UNKNOWN;
            FSM_Transition(fsm, I2C_FSM_STATE_CALLBACK);
            break;
    }
}

static void i2c_fsm_write_enter(FSM *fsm) {
    if (!start_write_phase(i2c.current)) {
        i2c.current->status = I2C_STATUS_ERROR_UNKNOWN;
        FSM_Transition(fsm, I2C_FSM_STATE_CALLBACK);
    }
}

static void i2c_fsm_write_service(FSM *fsm) {
    if (i2c.transfer_error) {
        i2c.transfer_error = false;
        i2c.current->status = i2c.error_status;
        FSM_Transition(fsm, I2C_FSM_STATE_CALLBACK);
        return;
    }

    if (!i2c.transfer_done) {
        return;
    }

    i2c.transfer_done = false;

    if (i2c.current->operation == I2C_OPERATION_WRITE) {
        FSM_Transition(fsm, I2C_FSM_STATE_CALLBACK);
    } else {
        FSM_Transition(fsm, I2C_FSM_STATE_READ);
    }
}

static void i2c_fsm_read_enter(FSM *fsm) {
    if (!start_read_phase(i2c.current)) {
        i2c.current->status = I2C_STATUS_ERROR_UNKNOWN;
        FSM_Transition(fsm, I2C_FSM_STATE_CALLBACK);
    }
}

static void i2c_fsm_read_service(FSM *fsm) {
    if (i2c.transfer_error) {
        i2c.transfer_error = false;
        i2c.current->status = i2c.error_status;
        FSM_Transition(fsm, I2C_FSM_STATE_CALLBACK);
        return;
    }

    if (!i2c.transfer_done) {
        return;
    }

    i2c.transfer_done = false;
    FSM_Transition(fsm, I2C_FSM_STATE_CALLBACK);
}

static void i2c_fsm_callback_enter(FSM *fsm) {
    if (i2c.hdma.Instance != NULL) {
        if (HAL_DMA_DeInit(&i2c.hdma) != HAL_OK) {
            Error_Handler();
        }
    }

    if (i2c.current->callback != NULL) {
        i2c.current = i2c.current->callback(i2c.current);
    } else {
        i2c.current = dequeue_transaction();
    }

    if (i2c.current != NULL) {
        i2c.current->queue_next = NULL;
        i2c.current->status = I2C_STATUS_SUCCESS;
        FSM_Transition(fsm, I2C_FSM_STATE_CONFIGURE);
    } else {
        FSM_Transition(fsm, I2C_FSM_STATE_IDLE);
    }
}

/**
  * @brief I2C MSP Initialization
  * This function configures the hardware resources used in this example
  * @param hi2c: I2C handle pointer
  * @retval None
  */
void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (hi2c->Instance == I2C2) {
        /* Initializes the peripherals clock */
        __HAL_RCC_I2C2_CONFIG(RCC_I2C2CLKSOURCE_PCLK1);

        __HAL_RCC_GPIOB_CLK_ENABLE();
        /* I2C2 GPIO Configuration
         * PB10     ------> I2C2_SCL
         * PB12     ------> I2C2_SDA
         */
        GPIO_InitStruct.Pin = I2C_SCL_Pin | I2C_SDA_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF4_I2C2;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        /* Peripheral clock enable */
        __HAL_RCC_I2C2_CLK_ENABLE();

        HAL_NVIC_SetPriority(GPDMA1_Channel2_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(GPDMA1_Channel2_IRQn);

        HAL_NVIC_SetPriority(I2C2_EV_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(I2C2_EV_IRQn);

        HAL_NVIC_SetPriority(I2C2_ER_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(I2C2_ER_IRQn);
    }
}

/**
  * @brief I2C MSP De-Initialization
  * This function freeze the hardware resources used in this example
  * @param hi2c: I2C handle pointer
  * @retval None
  */
void HAL_I2C_MspDeInit(I2C_HandleTypeDef *hi2c) {
    if (hi2c->Instance == I2C2) {
        /* Peripheral clock disable */
        __HAL_RCC_I2C2_CLK_DISABLE();

        /* I2C2 GPIO Configuration
         * PB10     ------> I2C2_SCL
         * PB12     ------> I2C2_SDA
         */
        HAL_GPIO_DeInit(GPIOB, I2C_SCL_Pin | I2C_SDA_Pin);

        HAL_NVIC_DisableIRQ(GPDMA1_Channel2_IRQn);
        HAL_NVIC_DisableIRQ(I2C2_EV_IRQn);
        HAL_NVIC_DisableIRQ(I2C2_ER_IRQn);
    }
}

void I2C_Init(void) {
    /* Initialise I2C2 peripheral at 100kHz. */
    i2c.hi2c.Instance = I2C2;
    i2c.hi2c.Init.Timing = 0x60808CD3;
    i2c.hi2c.Init.OwnAddress1 = 0;
    i2c.hi2c.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    i2c.hi2c.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    i2c.hi2c.Init.OwnAddress2 = 0;
    i2c.hi2c.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    i2c.hi2c.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    i2c.hi2c.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&i2c.hi2c) != HAL_OK) {
        Error_Handler();
    }

    if (HAL_I2CEx_ConfigAnalogFilter(&i2c.hi2c, 0) != HAL_OK) {
        Error_Handler();
    }

    if (HAL_I2CEx_ConfigDigitalFilter(&i2c.hi2c, 0) != HAL_OK) {
        Error_Handler();
    }

    if (!FSM_Init(&i2c.fsm, i2c_fsm_states, I2C_FSM_STATE_IDLE, NULL)) {
        Error_Handler();
    }
}

void I2C_Service(void) {
    FSM_Service(&i2c.fsm);
}

void I2C_Queue(I2C_Transaction *tx) {
    if (tx == NULL) {
        return;
    }

    tx->queue_next = NULL;
    tx->status = I2C_STATUS_SUCCESS;

    if (i2c.queue_tail == NULL) {
        i2c.queue_head = tx;
        i2c.queue_tail = tx;
    } else {
        i2c.queue_tail->queue_next = tx;
        i2c.queue_tail = tx;
    }
}

void GPDMA1_Channel2_IRQHandler(void) {
    HAL_DMA_IRQHandler(&i2c.hdma);
}

void I2C2_EV_IRQHandler(void) {
    HAL_I2C_EV_IRQHandler(&i2c.hi2c);
}

void I2C2_ER_IRQHandler(void) {
    HAL_I2C_ER_IRQHandler(&i2c.hi2c);
}

void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c) {
    if (hi2c->Instance == I2C2) {
        i2c.transfer_done = true;
    }
}

void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c) {
    if (hi2c->Instance == I2C2) {
        i2c.transfer_done = true;
    }
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c) {
    if (hi2c->Instance == I2C2) {
        i2c.error_status = i2c_hal_error_status();
        i2c.transfer_error = true;
    }
}
