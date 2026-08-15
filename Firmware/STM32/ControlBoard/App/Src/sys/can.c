#include "sys/can.h"
#include "sys/gpio.h"
#include "stm32h5xx_hal.h"
#include "stm32h5xx_it.h"
#include <stdbool.h>
#include <string.h>

#define CAN_TX_FIFO_LENGTH 3U
#define CAN_DLC_LOOKUP_LENGTH 16U
#define CAN_ACTIVITY_MS 5U
#define CAN_TIMESTAMP_WRAP_US 65536U
#ifndef CAN_FLASH_ON_RX
#define CAN_FLASH_ON_RX true
#endif

typedef struct {
    uint8_t length;
    uint8_t data[64];
    uint32_t bufferIndex;
    uint32_t queuedAtUs;
    bool active;
} CAN_TxShadow;

typedef struct {
    FDCAN_HandleTypeDef handle;
    volatile bool rxPending;
    volatile bool txEventPending;
    volatile uint32_t txEventsLost;
    volatile uint32_t timestampBaseUs;
    uint32_t activityUntil;
    CAN_TxShadow txShadow[CAN_TX_FIFO_LENGTH];
} can_t;

static can_t can;

static const uint8_t dlc_lengths[CAN_DLC_LOOKUP_LENGTH] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64,
};

static uint32_t length_to_dlc(uint8_t length) {
    for (uint8_t index = 0; index < CAN_DLC_LOOKUP_LENGTH; index++) {
        if (length <= dlc_lengths[index]) {
            return index;
        }
    }

    return FDCAN_DLC_BYTES_64;
}

static uint8_t dlc_to_length(uint32_t dlc) {
    if (dlc < CAN_DLC_LOOKUP_LENGTH) {
        return dlc_lengths[dlc];
    }

    return 0;
}

static uint32_t current_can_timestamp_in_us(void) {
    const uint32_t primask = __get_PRIMASK();
    uint32_t wrapPendingBefore;
    uint32_t wrapPendingAfter;
    uint16_t timestamp;

    __disable_irq();

    do {
        wrapPendingBefore = __HAL_FDCAN_GET_FLAG(&can.handle, FDCAN_FLAG_TIMESTAMP_WRAPAROUND);
        timestamp = HAL_FDCAN_GetTimestampCounter(&can.handle);
        wrapPendingAfter = __HAL_FDCAN_GET_FLAG(&can.handle, FDCAN_FLAG_TIMESTAMP_WRAPAROUND);
    } while (wrapPendingBefore != wrapPendingAfter);

    uint32_t timestampBaseUs = can.timestampBaseUs;
    if (wrapPendingAfter != 0U) {
        timestampBaseUs += CAN_TIMESTAMP_WRAP_US;
    }

    if (primask == 0U) {
        __enable_irq();
    }

    return timestampBaseUs + timestamp;
}

static void flash_activity_led(uint32_t durationMs) {
    HAL_GPIO_WritePin(FDCAN_ACT_Port, FDCAN_ACT_Pin, GPIO_PIN_RESET);
    can.activityUntil = HAL_GetTick() + durationMs;
}

static void service_activity_led(void) {
    if ((HAL_FDCAN_GetTxFifoFreeLevel(&can.handle) == CAN_TX_FIFO_LENGTH) &&
        ((int32_t) (HAL_GetTick() - can.activityUntil) >= 0)) {
        HAL_GPIO_WritePin(FDCAN_ACT_Port, FDCAN_ACT_Pin, GPIO_PIN_SET);
    }
}

/**
  * @brief FDCAN MSP Initialization
  * This function configures the hardware resources used in this example
  * @param hfdcan: FDCAN handle pointer
  * @retval None
  */
void HAL_FDCAN_MspInit(FDCAN_HandleTypeDef *hfdcan) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (hfdcan->Instance == FDCAN1) {
        __HAL_RCC_FDCAN_CONFIG(RCC_FDCANCLKSOURCE_PLL2Q);
        __HAL_RCC_FDCAN_CLK_ENABLE();

        /* FDCAN1 GPIO Configuration
         * PD0     ------> FDCAN1_RX
         * PD1     ------> FDCAN1_TX
         */
        GPIO_InitStruct.Pin = FDCAN_RX_Pin | FDCAN_TX_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF9_FDCAN1;
        HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

        HAL_NVIC_SetPriority(FDCAN1_IT0_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(FDCAN1_IT0_IRQn);
    }
}

/**
  * @brief FDCAN MSP De-Initialization
  * This function freeze the hardware resources used in this example
  * @param hfdcan: FDCAN handle pointer
  * @retval None
  */
void HAL_FDCAN_MspDeInit(FDCAN_HandleTypeDef *hfdcan) {
    if (hfdcan->Instance == FDCAN1) {
        /* Peripheral clock disable */
        __HAL_RCC_FDCAN_CLK_DISABLE();

        /* FDCAN1 GPIO Configuration
         * PD0     ------> FDCAN1_RX
         * PD1     ------> FDCAN1_TX
         */
        HAL_GPIO_DeInit(GPIOD, FDCAN_RX_Pin | FDCAN_TX_Pin);

        HAL_NVIC_DisableIRQ(FDCAN1_IT0_IRQn);
    }
}

void CAN_Init(void) {
    memset(&can, 0, sizeof(can));

    can.handle.Instance = FDCAN1;
    can.handle.Init.ClockDivider = FDCAN_CLOCK_DIV1;
    can.handle.Init.FrameFormat = FDCAN_FRAME_FD_NO_BRS;
    can.handle.Init.Mode = FDCAN_MODE_NORMAL;
    can.handle.Init.AutoRetransmission = DISABLE;
    can.handle.Init.TransmitPause = DISABLE;
    can.handle.Init.ProtocolException = DISABLE;

    can.handle.Init.NominalPrescaler = 1;
    can.handle.Init.NominalSyncJumpWidth = 16;
    can.handle.Init.NominalTimeSeg1 = 63;
    can.handle.Init.NominalTimeSeg2 = 16;

    // Same rate since BRS is off — mirror nominal values
    can.handle.Init.DataPrescaler = can.handle.Init.NominalPrescaler;
    can.handle.Init.DataSyncJumpWidth = can.handle.Init.NominalSyncJumpWidth;
    can.handle.Init.DataTimeSeg1 = can.handle.Init.NominalTimeSeg1;
    can.handle.Init.DataTimeSeg2 = can.handle.Init.NominalTimeSeg2;

    can.handle.Init.StdFiltersNbr = 0;
    can.handle.Init.ExtFiltersNbr = 0;
    can.handle.Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;

    if (HAL_FDCAN_Init(&can.handle) != HAL_OK) {
        Error_Handler();
    }

    if (HAL_FDCAN_ConfigGlobalFilter(&can.handle,
                                     FDCAN_ACCEPT_IN_RX_FIFO0,
                                     FDCAN_ACCEPT_IN_RX_FIFO0,
                                     FDCAN_REJECT_REMOTE,
                                     FDCAN_REJECT_REMOTE) != HAL_OK) {
        Error_Handler();
    }

    if ((HAL_FDCAN_ConfigTimestampCounter(&can.handle, FDCAN_TIMESTAMP_PRESC_1) != HAL_OK) ||
        (HAL_FDCAN_EnableTimestampCounter(&can.handle, FDCAN_TIMESTAMP_INTERNAL) != HAL_OK)) {
        Error_Handler();
    }

    const uint32_t notifications = FDCAN_IT_RX_FIFO0_NEW_MESSAGE |
                                   FDCAN_IT_TX_EVT_FIFO_NEW_DATA |
                                   FDCAN_IT_TX_EVT_FIFO_ELT_LOST |
                                   FDCAN_IT_TIMESTAMP_WRAPAROUND;

    if (HAL_FDCAN_ConfigInterruptLines(&can.handle,
                                       notifications,
                                       FDCAN_INTERRUPT_LINE0) != HAL_OK) {
        Error_Handler();
    }

    if (HAL_FDCAN_ActivateNotification(&can.handle,
                                       notifications,
                                       0) != HAL_OK) {
        Error_Handler();
    }

    if (HAL_FDCAN_Start(&can.handle) != HAL_OK) {
        Error_Handler();
    }

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Default value to high, LED off */
    HAL_GPIO_WritePin(FDCAN_ACT_Port, FDCAN_ACT_Pin, GPIO_PIN_SET);

    /* Configure LED */
    GPIO_InitStruct.Pin = FDCAN_ACT_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(FDCAN_ACT_Port, &GPIO_InitStruct);
}

void CAN_Service(CAN_PacketHandler packetHandler) {
    service_activity_led();

    if (can.rxPending ||
        (HAL_FDCAN_GetRxFifoFillLevel(&can.handle, FDCAN_RX_FIFO0) > 0U)) {
        can.rxPending = false;

        while (HAL_FDCAN_GetRxFifoFillLevel(&can.handle, FDCAN_RX_FIFO0) > 0) {
            FDCAN_RxHeaderTypeDef rxHeader;
            uint8_t rxData[64];

            if (HAL_FDCAN_GetRxMessage(&can.handle, FDCAN_RX_FIFO0, &rxHeader, rxData) != HAL_OK) {
                break;
            }

            if (CAN_FLASH_ON_RX) {
                flash_activity_led(CAN_ACTIVITY_MS);
            }

            if ((packetHandler != NULL) &&
                (rxHeader.RxFrameType == FDCAN_DATA_FRAME) &&
                (rxHeader.IdType == FDCAN_STANDARD_ID)) {
                const uint8_t length = dlc_to_length(rxHeader.DataLength);
                const uint32_t timestampNowUs = current_can_timestamp_in_us();
                const uint16_t sofToProcessingUs = (uint16_t) timestampNowUs -
                                                   (uint16_t) rxHeader.RxTimestamp;

                CAN_Packet packet = {
                    .identifier = (uint16_t) rxHeader.Identifier,
                    .length = length,
                    .data = rxData,
                    .direction = CAN_DIRECTION_IN,
                    .timing = {
                        .sofToProcessingUs = sofToProcessingUs,
                    },
                };
                packetHandler(&packet);
            }
        }
    }

    if (can.txEventPending ||
        ((can.handle.Instance->TXEFS & FDCAN_TXEFS_EFFL) != 0U)) {
        can.txEventPending = false;

        while ((can.handle.Instance->TXEFS & FDCAN_TXEFS_EFFL) != 0U) {
            FDCAN_TxEventFifoTypeDef txEvent;

            if (HAL_FDCAN_GetTxEvent(&can.handle, &txEvent) != HAL_OK) {
                break;
            }

            if (txEvent.MessageMarker >= CAN_TX_FIFO_LENGTH) {
                continue;
            }

            CAN_TxShadow *shadow = &can.txShadow[txEvent.MessageMarker];
            if (!shadow->active) {
                continue;
            }

            const uint32_t timestampNowUs = current_can_timestamp_in_us();
            const uint16_t sinceStartUs = (uint16_t) timestampNowUs -
                                          (uint16_t) txEvent.TxTimestamp;
            const uint32_t startedAtUs = timestampNowUs - sinceStartUs;
            const uint32_t queueToSofUs = startedAtUs - shadow->queuedAtUs;

            const uint8_t length = shadow->length;
            uint8_t txData[64];
            memcpy(txData, shadow->data, length);
            shadow->active = false;

            if (packetHandler != NULL) {
                CAN_Packet packet = {
                    .identifier = (uint16_t) txEvent.Identifier,
                    .length = length,
                    .data = txData,
                    .direction = CAN_DIRECTION_OUT,
                    .timing = {
                        .queueToSofUs = queueToSofUs,
                    },
                };
                packetHandler(&packet);
            }
        }
    }

    for (uint8_t index = 0U; index < CAN_TX_FIFO_LENGTH; index++) {
        CAN_TxShadow *shadow = &can.txShadow[index];

        if (shadow->active &&
            !HAL_FDCAN_IsTxBufferMessagePending(&can.handle, shadow->bufferIndex)) {
            shadow->active = false;
        }
    }
}

void CAN_Queue(uint16_t identifier, uint8_t length, void *data) {
    if ((length > 64) || (data == NULL)) {
        return;
    }

    if (HAL_FDCAN_GetTxFifoFreeLevel(&can.handle) == 0U) {
        return;
    }

    uint8_t marker;
    for (marker = 0U; marker < CAN_TX_FIFO_LENGTH; marker++) {
        if (!can.txShadow[marker].active) {
            break;
        }
    }

    if (marker == CAN_TX_FIFO_LENGTH) {
        return;
    }

    FDCAN_TxHeaderTypeDef txHeader = {0};
    uint8_t txData[64] = {0};

    txHeader.Identifier = identifier & 0x7FFU;
    txHeader.IdType = FDCAN_STANDARD_ID;
    txHeader.TxFrameType = FDCAN_DATA_FRAME;
    txHeader.DataLength = length_to_dlc(length);
    txHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    txHeader.BitRateSwitch = FDCAN_BRS_OFF;
    txHeader.FDFormat = FDCAN_FD_CAN;
    txHeader.TxEventFifoControl = FDCAN_STORE_TX_EVENTS;
    txHeader.MessageMarker = marker;

    memcpy(txData, data, length);

    const uint32_t queuedAtUs = current_can_timestamp_in_us();
    if (HAL_FDCAN_AddMessageToTxFifoQ(&can.handle, &txHeader, txData) == HAL_OK) {
        flash_activity_led(CAN_ACTIVITY_MS);

        const uint8_t frameLength = dlc_to_length(txHeader.DataLength);
        can.txShadow[marker].length = frameLength;
        memcpy(can.txShadow[marker].data, txData, frameLength);
        can.txShadow[marker].bufferIndex = HAL_FDCAN_GetLatestTxFifoQRequestBuffer(&can.handle);
        can.txShadow[marker].queuedAtUs = queuedAtUs;
        can.txShadow[marker].active = true;
    }
}

void FDCAN1_IT0_IRQHandler(void) {
    HAL_FDCAN_IRQHandler(&can.handle);
}

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs) {
    if ((hfdcan->Instance == FDCAN1) && ((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != 0U)) {
        can.rxPending = true;
    }
}

void HAL_FDCAN_TxEventFifoCallback(FDCAN_HandleTypeDef *hfdcan, uint32_t TxEventFifoITs) {
    if (hfdcan->Instance != FDCAN1) {
        return;
    }

    if ((TxEventFifoITs & FDCAN_IT_TX_EVT_FIFO_NEW_DATA) != 0U) {
        can.txEventPending = true;
    }

    if ((TxEventFifoITs & FDCAN_IT_TX_EVT_FIFO_ELT_LOST) != 0U) {
        can.txEventsLost++;
    }
}

void HAL_FDCAN_TimestampWraparoundCallback(FDCAN_HandleTypeDef *hfdcan) {
    if (hfdcan->Instance == FDCAN1) {
        can.timestampBaseUs += CAN_TIMESTAMP_WRAP_US;
    }
}
