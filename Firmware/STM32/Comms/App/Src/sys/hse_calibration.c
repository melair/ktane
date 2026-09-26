/**
  ******************************************************************************
  * @file           : hse_calibration.c
  * @brief          : HSE calibration from the external 1 Hz reference
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
#include "sys/hse_calibration.h"

#include "main.h"

#define HSE_TARGET_CYCLES_PER_SECOND       32000000U
#define HSE_CALIBRATION_START_PULSES       5U
#define HSE_CALIBRATION_SETTLE_SAMPLES     2U
#define HSE_CALIBRATION_AVERAGE_SAMPLES    5U
#define HSE_CALIBRATION_TOLERANCE_CYCLES   16

static TIM_HandleTypeDef htim2;

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

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
void HSE_Calibration_Init(void) {

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
void HSE_Calibration_Service(void) {
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
  * @brief Start capture and overflow interrupts after peripheral initialization.
  */
void HSE_Calibration_Start(void) {
    if (HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1) != HAL_OK) {
        Error_Handler();
    }

    /* Generate an IRQ on each 16-bit wrap so TIM2 forms a 32-bit time base. */
    __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);
    __HAL_TIM_ENABLE_IT(&htim2, TIM_IT_UPDATE);
}

/**
  * @brief TIM_Base MSP Initialization
  * This function configures the hardware resources used in this example
  * @param htim_base: TIM_Base handle pointer
  * @retval None
  */
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim_base) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (htim_base->Instance == TIM2) {

        /* Peripheral clock enable */
        __HAL_RCC_TIM2_CLK_ENABLE();

        __HAL_RCC_GPIOB_CLK_ENABLE();
        /**TIM2 GPIO Configuration
    PB4     ------> TIM2_CH1
    */
        GPIO_InitStruct.Pin = CAL_IN_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF4_TIM2;
        HAL_GPIO_Init(CAL_IN_GPIO_Port, &GPIO_InitStruct);

        HAL_PWREx_DisableGPIOPullUp(PWR_GPIO_B, PWR_GPIO_BIT_4);

        HAL_PWREx_DisableGPIOPullDown(PWR_GPIO_B, PWR_GPIO_BIT_4);

        HAL_NVIC_SetPriority(TIM2_IRQn, 3, 0);
        HAL_NVIC_EnableIRQ(TIM2_IRQn);

    }
}

/**
  * @brief TIM_Base MSP De-Initialization
  * This function freeze the hardware resources used in this example
  * @param htim_base: TIM_Base handle pointer
  * @retval None
  */
void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef *htim_base) {
    if (htim_base->Instance == TIM2) {

        /* Peripheral clock disable */
        __HAL_RCC_TIM2_CLK_DISABLE();

        /**TIM2 GPIO Configuration
    PB4     ------> TIM2_CH1
    */
        HAL_GPIO_DeInit(CAL_IN_GPIO_Port, CAL_IN_Pin);

        HAL_NVIC_DisableIRQ(TIM2_IRQn);

    }
}

/**
  * @brief This function handles TIM2 global interrupt.
  */
void TIM2_IRQHandler(void) {
    HAL_TIM_IRQHandler(&htim2);
}
