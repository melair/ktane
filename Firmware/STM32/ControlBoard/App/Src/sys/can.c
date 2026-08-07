#include "sys/can.h"
#include "sys/gpio.h"
#include "stm32h5xx_hal.h"
#include "stm32h5xx_it.h"
#include <stdbool.h>
#include <string.h>

#define CAN_TX_FIFO_LENGTH 3U
#define CAN_DLC_LOOKUP_LENGTH 16U
#define CAN_ACTIVITY_MS 5U
#ifndef CAN_FLASH_ON_RX
#define CAN_FLASH_ON_RX true
#endif

typedef struct {
    uint8_t length;
    uint32_t dlc;
} CAN_DlcLookup;

typedef struct {
    FDCAN_HandleTypeDef handle;
    volatile bool rxPending;
    uint32_t activityUntil;
} can_t;

static can_t can;

static const CAN_DlcLookup dlcLookup[CAN_DLC_LOOKUP_LENGTH] = {
    {0, FDCAN_DLC_BYTES_0},
    {1, FDCAN_DLC_BYTES_1},
    {2, FDCAN_DLC_BYTES_2},
    {3, FDCAN_DLC_BYTES_3},
    {4, FDCAN_DLC_BYTES_4},
    {5, FDCAN_DLC_BYTES_5},
    {6, FDCAN_DLC_BYTES_6},
    {7, FDCAN_DLC_BYTES_7},
    {8, FDCAN_DLC_BYTES_8},
    {12, FDCAN_DLC_BYTES_12},
    {16, FDCAN_DLC_BYTES_16},
    {20, FDCAN_DLC_BYTES_20},
    {24, FDCAN_DLC_BYTES_24},
    {32, FDCAN_DLC_BYTES_32},
    {48, FDCAN_DLC_BYTES_48},
    {64, FDCAN_DLC_BYTES_64},
};

static uint32_t lengthToDlc(uint8_t length) {
    for (uint8_t index = 8; index < CAN_DLC_LOOKUP_LENGTH; index++) {
        if (length <= dlcLookup[index].length) {
            return dlcLookup[index].dlc;
        }
    }

    return FDCAN_DLC_BYTES_64;
}

static uint8_t dlcToLength(uint32_t dlc) {
    if (dlc < CAN_DLC_LOOKUP_LENGTH) {
        return dlcLookup[dlc].length;
    }

    return 0;
}

static void flashActivityLed(can_t *canBus, uint32_t durationMs) {
    HAL_GPIO_WritePin(FDCAN_ACT_Port, FDCAN_ACT_Pin, GPIO_PIN_RESET);
    canBus->activityUntil = HAL_GetTick() + durationMs;
}

static void serviceActivityLed(can_t *canBus) {
    if ((HAL_FDCAN_GetTxFifoFreeLevel(&canBus->handle) == CAN_TX_FIFO_LENGTH) &&
        ((int32_t) (HAL_GetTick() - canBus->activityUntil) >= 0)) {
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

    if (HAL_FDCAN_ConfigInterruptLines(&can.handle,
                                       FDCAN_IT_RX_FIFO0_NEW_MESSAGE,
                                       FDCAN_INTERRUPT_LINE0) != HAL_OK) {
        Error_Handler();
    }

    if (HAL_FDCAN_ActivateNotification(&can.handle,
                                       FDCAN_IT_RX_FIFO0_NEW_MESSAGE,
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

void CAN_Service(void (*packetHandler)(uint16_t mailbox, uint8_t length, void *data)) {
    serviceActivityLed(&can);

    if (!can.rxPending) {
        return;
    }

    while (HAL_FDCAN_GetRxFifoFillLevel(&can.handle, FDCAN_RX_FIFO0) > 0) {
        FDCAN_RxHeaderTypeDef rxHeader;
        uint8_t rxData[64];

        if (HAL_FDCAN_GetRxMessage(&can.handle, FDCAN_RX_FIFO0, &rxHeader, rxData) != HAL_OK) {
            break;
        }

        if (CAN_FLASH_ON_RX) {
            flashActivityLed(&can, CAN_ACTIVITY_MS);
        }

        if ((packetHandler != NULL) &&
            (rxHeader.RxFrameType == FDCAN_DATA_FRAME) &&
            (rxHeader.IdType == FDCAN_STANDARD_ID)) {
            const uint8_t length = dlcToLength(rxHeader.DataLength);

            if ((length >= 8) && (length <= 64)) {
                packetHandler((uint16_t) rxHeader.Identifier, length, rxData);
            }
        }
    }

    can.rxPending = HAL_FDCAN_GetRxFifoFillLevel(&can.handle, FDCAN_RX_FIFO0) > 0;
}

void CAN_Queue(uint16_t mailbox, uint8_t length, void *data) {
    if ((length < 8) || (length > 64) || (data == NULL)) {
        return;
    }

    if (HAL_FDCAN_GetTxFifoFreeLevel(&can.handle) == 0U) {
        return;
    }

    FDCAN_TxHeaderTypeDef txHeader = {0};
    uint8_t txData[64] = {0};

    txHeader.Identifier = mailbox & 0x7FFU;
    txHeader.IdType = FDCAN_STANDARD_ID;
    txHeader.TxFrameType = FDCAN_DATA_FRAME;
    txHeader.DataLength = lengthToDlc(length);
    txHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    txHeader.BitRateSwitch = FDCAN_BRS_OFF;
    txHeader.FDFormat = FDCAN_FD_CAN;
    txHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    txHeader.MessageMarker = 0;

    memcpy(txData, data, length);

    flashActivityLed(&can, CAN_ACTIVITY_MS);

    if (HAL_FDCAN_AddMessageToTxFifoQ(&can.handle, &txHeader, txData) != HAL_OK) {
        serviceActivityLed(&can);
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
