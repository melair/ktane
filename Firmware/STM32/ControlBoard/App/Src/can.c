#include "can.h"
#include "gpio.h"
#include "stm32h5xx_hal.h"
#include "stm32h5xx_it.h"

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
        GPIO_InitStruct.Pin = FDCAN_RX_Pin | FDCAN_RX_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF9_FDCAN1;
        HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
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
        HAL_GPIO_DeInit(GPIOD, FDCAN_RX_Pin | FDCAN_RX_Pin);
    }
}

void CAN_Init(void) {
    FDCAN_HandleTypeDef hfdcan1;

    hfdcan1.Instance = FDCAN1;
    hfdcan1.Init.ClockDivider = FDCAN_CLOCK_DIV1;
    hfdcan1.Init.FrameFormat = FDCAN_FRAME_FD_NO_BRS;
    hfdcan1.Init.Mode = FDCAN_MODE_NORMAL;
    hfdcan1.Init.AutoRetransmission = DISABLE;
    hfdcan1.Init.TransmitPause = DISABLE;
    hfdcan1.Init.ProtocolException = DISABLE;

    hfdcan1.Init.NominalPrescaler = 1;
    hfdcan1.Init.NominalSyncJumpWidth = 16;
    hfdcan1.Init.NominalTimeSeg1 = 63;
    hfdcan1.Init.NominalTimeSeg2 = 16;

    // Same rate since BRS is off — mirror nominal values
    hfdcan1.Init.DataPrescaler = hfdcan1.Init.NominalPrescaler;
    hfdcan1.Init.DataSyncJumpWidth = hfdcan1.Init.NominalSyncJumpWidth ;
    hfdcan1.Init.DataTimeSeg1 = hfdcan1.Init.NominalTimeSeg1;
    hfdcan1.Init.DataTimeSeg2 = hfdcan1.Init.NominalTimeSeg2;

    hfdcan1.Init.StdFiltersNbr = 0;
    hfdcan1.Init.ExtFiltersNbr = 0;
    hfdcan1.Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;

    if (HAL_FDCAN_Init(&hfdcan1) != HAL_OK) {
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
