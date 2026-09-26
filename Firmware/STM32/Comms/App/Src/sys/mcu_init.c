/**
  ******************************************************************************
  * @file           : mcu_init.c
  * @brief          : MCU clock and GPIO initialization.
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
#include "sys/mcu_init.h"

#include "main.h"

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Init(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* Initialize the HSE and LSE oscillators. */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_LSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.LSEState = RCC_LSE_ON;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    /* TIM2 is clocked from HCLK; select direct HSE so one timer tick is one HSE cycle. */
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_DIRECT_HSE;
    RCC_ClkInitStruct.SYSCLKDivider = RCC_DIRECT_HSE_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_WAIT_STATES_1) != HAL_OK) {
        Error_Handler();
    }
    HAL_RCC_MCOConfig(RCC_MCO1, RCC_MCO1SOURCE_HSE, RCC_MCODIV_1);
}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeripheralClock_Init(void) {
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

    /* Configure the SMPS clock. */
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SMPS;
    PeriphClkInitStruct.SmpsDivSelection = RCC_SMPSCLK_DIV4;

    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
        Error_Handler();
    }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
void GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Enable clocks for all GPIO ports available on STM32WB09. */
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(RUN_GPIO_Port, RUN_Pin, GPIO_PIN_RESET);

    /*Configure GPIO pin : BLE_INT_Pin */
    GPIO_InitStruct.Pin = BLE_INT_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(BLE_INT_GPIO_Port, &GPIO_InitStruct);

    /*Configure GPIO pin : PA2 */
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF7_SWDIO;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /*Configure GPIO pin : PB14 */
    GPIO_InitStruct.Pin = GPIO_PIN_14;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF3_MCO;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /*Configure GPIO pin : RUN_Pin */
    GPIO_InitStruct.Pin = RUN_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(RUN_GPIO_Port, &GPIO_InitStruct);

    /* Preserve the board's GPIO pull configuration in low-power modes. */
    HAL_PWREx_DisableGPIOPullUp(PWR_GPIO_B, PWR_GPIO_BIT_2 | PWR_GPIO_BIT_14 | PWR_GPIO_BIT_5);

    HAL_PWREx_DisableGPIOPullDown(PWR_GPIO_B, PWR_GPIO_BIT_2 | PWR_GPIO_BIT_14 | PWR_GPIO_BIT_5);

    HAL_PWREx_EnableGPIOPullUp(PWR_GPIO_A, PWR_GPIO_BIT_2);
}
