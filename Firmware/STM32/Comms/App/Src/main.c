/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "sys/mcu_init.h"
#include "sys/hse_calibration.h"
#include "sys/rtc.h"

/* Private includes ----------------------------------------------------------*/
#include "app_ble.h"
#include "app_conf.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

UART_HandleTypeDef hlpuart1;

PKA_HandleTypeDef hpka;

RNG_HandleTypeDef hrng;

/* Private function prototypes -----------------------------------------------*/
static void MX_LPUART1_UART_Init(void);

static void MX_RADIO_Init(void);

static void MX_RADIO_TIMER_Init(void);

static void MX_RNG_Init(void);

static void MX_PKA_Init(void);

/* Private user code ---------------------------------------------------------*/

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void) {

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Init();

  /* Configure the peripherals common clocks */
  PeripheralClock_Init();

  /* Initialize all configured peripherals */
  GPIO_Init();
  MX_LPUART1_UART_Init();
  MX_RADIO_Init();
  MX_RADIO_TIMER_Init();
  HSE_Calibration_Init();
  MX_RNG_Init();
  MX_PKA_Init();
  RTC_Init();

  HSE_Calibration_Start();

  /* Init code for STM32_BLE */
  MX_APPE_Init(NULL);

  APP_BLE_SetPairingMode(0U);

  /* Start connectable BLE advertising once the GAP/GATT database is ready. */
  APP_BLE_Procedure_Gap_Peripheral(PROC_GAP_PERIPH_ADVERTISE_START_FAST);

  /* Infinite loop */
  while (1) {
    MX_APPE_Process();

    HSE_Calibration_Service();
  }
}


/**
  * @brief LPUART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_LPUART1_UART_Init(void) {

  hlpuart1.Instance = LPUART1;
  hlpuart1.Init.BaudRate = 115200;
  hlpuart1.Init.WordLength = UART_WORDLENGTH_8B;
  hlpuart1.Init.StopBits = UART_STOPBITS_1;
  hlpuart1.Init.Parity = UART_PARITY_NONE;
  hlpuart1.Init.Mode = UART_MODE_TX_RX;
  hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  hlpuart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  hlpuart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&hlpuart1) != HAL_OK) {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&hlpuart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK) {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&hlpuart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK) {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&hlpuart1) != HAL_OK) {
    Error_Handler();
  }

}

/**
  * @brief PKA Initialization Function
  * @param None
  * @retval None
  */
static void MX_PKA_Init(void) {

  hpka.Instance = PKA;
  if (HAL_PKA_Init(&hpka) != HAL_OK) {
    Error_Handler();
  }

}

/**
  * @brief RADIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_RADIO_Init(void) {

  RADIO_HandleTypeDef hradio = {0};

  if (__HAL_RCC_RADIO_IS_CLK_DISABLED()) {
    /* Radio Peripheral reset */
    __HAL_RCC_RADIO_FORCE_RESET();
    __HAL_RCC_RADIO_RELEASE_RESET();

    /* Enable Radio peripheral clock */
    __HAL_RCC_RADIO_CLK_ENABLE();
  }
  hradio.Instance = RADIO;
  HAL_RADIO_Init(&hradio);

}

/**
  * @brief RADIO_TIMER Initialization Function
  * @param None
  * @retval None
  */
static void MX_RADIO_TIMER_Init(void) {

  RADIO_TIMER_InitTypeDef RADIO_TIMER_InitStruct = {0};

  if (__HAL_RCC_RADIO_IS_CLK_DISABLED()) {
    /* Radio Peripheral reset */
    __HAL_RCC_RADIO_FORCE_RESET();
    __HAL_RCC_RADIO_RELEASE_RESET();

    /* Enable Radio peripheral clock */
    __HAL_RCC_RADIO_CLK_ENABLE();
  }
  /* Wait to be sure that the Radio Timer is active */
  while (LL_RADIO_TIMER_GetAbsoluteTime(WAKEUP) < 0x10);
  RADIO_TIMER_InitStruct.XTAL_StartupTime = 320;
  RADIO_TIMER_InitStruct.enableInitialCalibration = FALSE;
  RADIO_TIMER_InitStruct.periodicCalibrationInterval = 0;
  HAL_RADIO_TIMER_Init(&RADIO_TIMER_InitStruct);

}

/**
  * @brief RNG Initialization Function
  * @param None
  * @retval None
  */
static void MX_RNG_Init(void) {

  hrng.Instance = RNG;
  if (HAL_RNG_Init(&hrng) != HAL_OK) {
    Error_Handler();
  }

}



/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void) {
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1) {
  }
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line) {
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
}
#endif /* USE_FULL_ASSERT */
