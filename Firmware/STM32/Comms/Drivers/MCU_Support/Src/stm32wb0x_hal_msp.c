/**
  ******************************************************************************
  * @file         stm32wb0x_hal_msp.c
  * @brief        This file provides code for the MSP Initialization
  *               and de-Initialization codes.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* External functions --------------------------------------------------------*/

/**
  * Initializes the Global MSP.
  */
void HAL_MspInit(void) {

    __HAL_RCC_SYSCFG_CLK_ENABLE();

    /* System interrupt init*/

}

/**
  * @brief UART MSP Initialization
  * This function configures the hardware resources used in this example
  * @param huart: UART handle pointer
  * @retval None
  */
void HAL_UART_MspInit(UART_HandleTypeDef *huart) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
    if (huart->Instance == LPUART1) {

        /** Initializes the peripherals clock
  */
        PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LPUART1;
        PeriphClkInitStruct.LPUART1ClockSelection = RCC_LPUART1_CLKSOURCE_16M;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
            Error_Handler();
        }

        /* Peripheral clock enable */
        __HAL_RCC_LPUART1_CLK_ENABLE();

        __HAL_RCC_GPIOB_CLK_ENABLE();
        /**LPUART1 GPIO Configuration
    PB7     ------> LPUART1_RX
    PB6     ------> LPUART1_TX
    */
        GPIO_InitStruct.Pin = GPIO_PIN_7 | GPIO_PIN_6;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF3_LPUART1;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        HAL_PWREx_DisableGPIOPullUp(PWR_GPIO_B, PWR_GPIO_BIT_7 | PWR_GPIO_BIT_6);

        HAL_PWREx_DisableGPIOPullDown(PWR_GPIO_B, PWR_GPIO_BIT_7 | PWR_GPIO_BIT_6);

        HAL_NVIC_SetPriority(LPUART1_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(LPUART1_IRQn);

    }
}

/**
  * @brief UART MSP De-Initialization
  * This function freeze the hardware resources used in this example
  * @param huart: UART handle pointer
  * @retval None
  */
void HAL_UART_MspDeInit(UART_HandleTypeDef *huart) {
    if (huart->Instance == LPUART1) {

        /* Peripheral clock disable */
        __HAL_RCC_LPUART1_CLK_DISABLE();

        /**LPUART1 GPIO Configuration
    PB7     ------> LPUART1_RX
    PB6     ------> LPUART1_TX
    */
        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_7 | GPIO_PIN_6);

        HAL_NVIC_DisableIRQ(LPUART1_IRQn);

    }
}

/**
  * @brief PKA MSP Initialization
  * This function configures the hardware resources used in this example
  * @param hpka: PKA handle pointer
  * @retval None
  */
void HAL_PKA_MspInit(PKA_HandleTypeDef *hpka) {
    if (hpka->Instance == PKA) {

        /* Peripheral clock enable */
        __HAL_RCC_PKA_CLK_ENABLE();
        /* Secure Connections key generation advances from the PKA completion IRQ. */
        HAL_NVIC_SetPriority(PKA_IRQn, 1, 0);
        HAL_NVIC_EnableIRQ(PKA_IRQn);

    }
}

/**
  * @brief PKA MSP De-Initialization
  * This function freeze the hardware resources used in this example
  * @param hpka: PKA handle pointer
  * @retval None
  */
void HAL_PKA_MspDeInit(PKA_HandleTypeDef *hpka) {
    if (hpka->Instance == PKA) {

        /* Peripheral clock disable */
        __HAL_RCC_PKA_CLK_DISABLE();
        HAL_NVIC_DisableIRQ(PKA_IRQn);

    }
}

/**
  * @brief RADIO MSP Initialization
  * This function configures the hardware resources used in this example
  * @param hradio: RADIO handle pointer
  * @retval None
  */
void HAL_RADIO_MspInit(RADIO_HandleTypeDef *hradio) {
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
    if (hradio->Instance == RADIO) {

        /** Initializes the peripherals clock
  */
        PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RF;
        PeriphClkInitStruct.RFClockSelection = RCC_RF_CLK_32M;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
            Error_Handler();
        }

        /* Peripheral clock enable */
        if (__HAL_RCC_RADIO_IS_CLK_DISABLED()) {
            /* Radio reset */
            __HAL_RCC_RADIO_FORCE_RESET();
            __HAL_RCC_RADIO_RELEASE_RESET();

            /* Enable Radio peripheral clock */
            __HAL_RCC_RADIO_CLK_ENABLE();
        }

        /**RADIO GPIO Configuration
    RF1     ------> RADIO_RF1
    */
        /* RADIO interrupt Init */
        HAL_NVIC_SetPriority(RADIO_TXRX_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(RADIO_TXRX_IRQn);
        HAL_NVIC_SetPriority(RADIO_TXRX_SEQ_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(RADIO_TXRX_SEQ_IRQn);
        HAL_NVIC_SetPriority(RADIO_RRM_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(RADIO_RRM_IRQn);

    }
}

/**
  * @brief RADIO MSP De-Initialization
  * This function freeze the hardware resources used in this example
  * @param hradio: RADIO handle pointer
  * @retval None
  */
void HAL_RADIO_MspDeInit(RADIO_HandleTypeDef *hradio) {
    if (hradio->Instance == RADIO) {

        /* Peripheral clock disable */
        __HAL_RCC_RADIO_CLK_DISABLE();
        __HAL_RCC_RADIO_FORCE_RESET();
        __HAL_RCC_RADIO_RELEASE_RESET();

        /* RADIO interrupt DeInit */
        HAL_NVIC_DisableIRQ(RADIO_TXRX_IRQn);
        HAL_NVIC_DisableIRQ(RADIO_TXRX_SEQ_IRQn);
        HAL_NVIC_DisableIRQ(RADIO_RRM_IRQn);

    }
}

/**
  * @brief RNG MSP Initialization
  * This function configures the hardware resources used in this example
  * @param hrng: RNG handle pointer
  * @retval None
  */
void HAL_RNG_MspInit(RNG_HandleTypeDef *hrng) {
    if (hrng->Instance == RNG) {

        /* Peripheral clock enable */
        __HAL_RCC_RNG_CLK_ENABLE();

    }
}

/**
  * @brief RNG MSP De-Initialization
  * This function freeze the hardware resources used in this example
  * @param hrng: RNG handle pointer
  * @retval None
  */
void HAL_RNG_MspDeInit(RNG_HandleTypeDef *hrng) {
    if (hrng->Instance == RNG) {

        /* Peripheral clock disable */
        __HAL_RCC_RNG_CLK_DISABLE();

    }
}
