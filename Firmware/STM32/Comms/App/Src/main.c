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

/* Private includes ----------------------------------------------------------*/
#include "app_ble.h"
#include "app_conf.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

#define HSE_TARGET_CYCLES_PER_SECOND       32000000U
#define HSE_CALIBRATION_START_PULSES       5U
#define HSE_CALIBRATION_SETTLE_SAMPLES     2U
#define HSE_CALIBRATION_AVERAGE_SAMPLES    5U
#define HSE_CALIBRATION_TOLERANCE_CYCLES   16

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

UART_HandleTypeDef hlpuart1;

PKA_HandleTypeDef hpka;

RNG_HandleTypeDef hrng;

RTC_HandleTypeDef hrtc;

TIM_HandleTypeDef htim2;

/* Updated after each complete 1 Hz period measured on TIM2_CH1 (PB4). */
volatile uint32_t hse_cycles_per_second = 0U;

/* TIM2 is a 16-bit timer; this software high word extends it to 32 bits. */
volatile uint32_t tim2_overflow_count = 0U;

/* Input-capture diagnostics, useful as debugger live-watch expressions. */
volatile uint32_t tim2_ch1_capture_count = 0U;
volatile uint32_t tim2_ch1_last_capture = 0U;
volatile uint32_t tim2_ch1_extended_capture = 0U;

/* HSE calibration state, exposed for debugger live-watch. */
volatile uint32_t hse_calibration_average_cycles = 0U;
volatile int32_t hse_calibration_error_cycles = 0;
volatile uint8_t hse_calibration_xotune = 0U;
volatile uint8_t hse_calibration_settle_remaining = HSE_CALIBRATION_SETTLE_SAMPLES;
volatile uint8_t hse_calibration_sample_count = 0U;
volatile uint8_t hse_calibration_adjustment_count = 0U;
volatile uint8_t hse_calibration_converged = 0U;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

void PeriphCommonClock_Config(void);

static void MX_GPIO_Init(void);

static void MX_LPUART1_UART_Init(void);

static void MX_RADIO_Init(void);

static void MX_RADIO_TIMER_Init(void);

static void MX_TIM2_Init(void);

static void MX_RNG_Init(void);

static void MX_PKA_Init(void);

static void MX_RTC_Init(void);

static void HSE_Calibration_Process(void);

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
  SystemClock_Config();

  /* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_LPUART1_UART_Init();
  MX_RADIO_Init();
  MX_RADIO_TIMER_Init();
  MX_TIM2_Init();
  MX_RNG_Init();
  MX_PKA_Init();
  MX_RTC_Init();

  if (HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1) != HAL_OK) {
    Error_Handler();
  }

  /* Generate an IRQ on each 16-bit wrap so TIM2 forms a 32-bit time base. */
  __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);
  __HAL_TIM_ENABLE_IT(&htim2, TIM_IT_UPDATE);

  /* Init code for STM32_BLE */
  MX_APPE_Init(NULL);

  /* Erase stored pairing information on every startup, before advertising. */
  tBleStatus clear_pairing_status = APP_BLE_ClearPairingInformation();
  APP_DBG_MSG("Clear pairing information: 0x%02X\n", clear_pairing_status);
  UNUSED(clear_pairing_status);

  /* Start locked; only peers in the bonded-device accept list may connect. */
  APP_BLE_SetPairingMode(1U);

  /* Start connectable BLE advertising once the GAP/GATT database is ready. */
  APP_BLE_Procedure_Gap_Peripheral(PROC_GAP_PERIPH_ADVERTISE_START_FAST);

  /* Infinite loop */
  while (1) {
    MX_APPE_Process();

    HSE_Calibration_Process();
  }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  /** Configure the SYSCLKSource and SYSCLKDivider
  */
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
void PeriphCommonClock_Config(void) {
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the peripherals clock
  */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SMPS;
  PeriphClkInitStruct.SmpsDivSelection = RCC_SMPSCLK_DIV4;

  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
    Error_Handler();
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
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void) {

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  if (HAL_RTC_Init(&hrtc) != HAL_OK) {
    Error_Handler();
  }

  /** Initialize RTC and set the Time and Date
  */
  sTime.Hours = 0x0;
  sTime.Minutes = 0x0;
  sTime.Seconds = 0x0;
  sTime.SubSeconds = 0x0;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK) {
    Error_Handler();
  }
  sDate.WeekDay = RTC_WEEKDAY_MONDAY;
  sDate.Month = RTC_MONTH_JANUARY;
  sDate.Date = 0x1;
  sDate.Year = 0x0;

  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK) {
    Error_Handler();
  }

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void) {

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_IC_InitTypeDef sConfigIC = {0};

  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 65535;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK) {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK) {
    Error_Handler();
  }
  if (HAL_TIM_IC_Init(&htim2) != HAL_OK) {
    Error_Handler();
  }
  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
  sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
  sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
  sConfigIC.ICFilter = 0;
  if (HAL_TIM_IC_ConfigChannel(&htim2, &sConfigIC, TIM_CHANNEL_1) != HAL_OK) {
    Error_Handler();
  }

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
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

  /**/
  HAL_PWREx_DisableGPIOPullUp(PWR_GPIO_B, PWR_GPIO_BIT_2 | PWR_GPIO_BIT_14 | PWR_GPIO_BIT_5);

  /**/
  HAL_PWREx_DisableGPIOPullDown(PWR_GPIO_B, PWR_GPIO_BIT_2 | PWR_GPIO_BIT_14 | PWR_GPIO_BIT_5);

  /**/
  HAL_PWREx_EnableGPIOPullUp(PWR_GPIO_A, PWR_GPIO_BIT_2);

}

/**
  * @brief  Stores the elapsed HSE cycles between successive rising 1 Hz edges.
  * @note   The first edge only establishes the initial capture timestamp.
  */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) {
  static uint32_t previous_capture_timestamp = 0U;
  static uint8_t have_previous_capture = 0U;
  uint32_t current_capture;
  uint32_t overflow_count;
  uint32_t capture_timestamp;

  if ((htim->Instance != TIM2) || (htim->Channel != HAL_TIM_ACTIVE_CHANNEL_1)) {
    return;
  }

  current_capture = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
  tim2_ch1_last_capture = current_capture;
  tim2_ch1_capture_count++;

  overflow_count = tim2_overflow_count;

  /*
   * HAL handles CC1 before UPDATE.  If an update is pending and the captured
   * count is in the lower half of the timer range, the wrap preceded the edge.
   */
  if (((TIM2->SR & TIM_SR_UIF) != 0U) && (current_capture < 0x8000U)) {
    overflow_count++;
  }

  capture_timestamp = (overflow_count << 16) | current_capture;
  tim2_ch1_extended_capture = capture_timestamp;

  if (have_previous_capture != 0U) {
    /* Unsigned subtraction also handles the extended 32-bit counter wrap. */
    hse_cycles_per_second = capture_timestamp - previous_capture_timestamp;
  }

  previous_capture_timestamp = capture_timestamp;
  have_previous_capture = 1U;
}

/**
  * @brief  Extends the TIM2 16-bit counter with a software high word.
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
  if (htim->Instance == TIM2) {
    tim2_overflow_count++;
  }
}

/**
  * @brief  Tunes the HSE capacitor bank from averaged 1 Hz reference captures.
  * @note   A larger XOTUNE code adds load capacitance and lowers HSE frequency.
  */
static void HSE_Calibration_Process(void) {
  static uint32_t observed_capture_count = 0U;
  static int64_t sample_sum = 0;
  static uint8_t initialized = 0U;
  uint32_t capture_count;
  uint32_t measured_cycles;

  capture_count = tim2_ch1_capture_count;
  if ((capture_count < HSE_CALIBRATION_START_PULSES) ||
      (capture_count == observed_capture_count)) {
    return;
  }

  if (initialized == 0U) {
    hse_calibration_xotune = (uint8_t) LL_RCC_HSE_GetCapacitorTuning();
    initialized = 1U;
  }

  observed_capture_count = capture_count;
  measured_cycles = hse_cycles_per_second;

  /* Discard the capture containing the change and allow one extra second to settle. */
  if (hse_calibration_settle_remaining != 0U) {
    hse_calibration_settle_remaining--;
    return;
  }

  sample_sum += measured_cycles;
  hse_calibration_sample_count++;
  if (hse_calibration_sample_count < HSE_CALIBRATION_AVERAGE_SAMPLES) {
    return;
  }

  hse_calibration_average_cycles =
      (uint32_t) (sample_sum / (int64_t) HSE_CALIBRATION_AVERAGE_SAMPLES);
  hse_calibration_error_cycles =
      (int32_t) hse_calibration_average_cycles - (int32_t) HSE_TARGET_CYCLES_PER_SECOND;
  sample_sum = 0;
  hse_calibration_sample_count = 0U;

  if ((hse_calibration_error_cycles <= HSE_CALIBRATION_TOLERANCE_CYCLES) &&
      (hse_calibration_error_cycles >= -HSE_CALIBRATION_TOLERANCE_CYCLES)) {
    hse_calibration_converged = 1U;
    return;
  }

  hse_calibration_converged = 0U;
  if (hse_calibration_error_cycles > 0) {
    /* HSE is fast: raise capacitance, which lowers its frequency. */
    if (hse_calibration_xotune < 63U) {
      hse_calibration_xotune++;
    } else {
      hse_calibration_converged = 1U;
      return;
    }
  } else {
    /* HSE is slow: reduce capacitance, which raises its frequency. */
    if (hse_calibration_xotune > 0U) {
      hse_calibration_xotune--;
    } else {
      hse_calibration_converged = 1U;
      return;
    }
  }

  LL_RCC_HSE_SetCapacitorTuning(hse_calibration_xotune);
  hse_calibration_adjustment_count++;
  hse_calibration_settle_remaining = HSE_CALIBRATION_SETTLE_SAMPLES;
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
