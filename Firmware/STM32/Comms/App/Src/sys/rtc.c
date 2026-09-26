/**
  ******************************************************************************
  * @file           : rtc.c
  * @brief          : Comms RTC initialization
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
#include "sys/rtc.h"

#include "main.h"

static RTC_HandleTypeDef hrtc = {0};

/**
  * @brief RTC MSP Initialization
  * This function configures the hardware resources used in this example
  * @param hrtc: RTC handle pointer
  * @retval None
  */
void HAL_RTC_MspInit(RTC_HandleTypeDef *hrtc) {
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
    if (hrtc->Instance == RTC) {

        /* Select LSE for the shared RTC, watchdog, and BLE wakeup clock. */
        PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC_WDG_BLEWKUP;
        PeriphClkInitStruct.RTCWDGBLEWKUPClockSelection = RCC_RTC_WDG_BLEWKUP_CLKSOURCE_LSE;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
            Error_Handler();
        }

        /* Peripheral clock enable */
        __HAL_RCC_RTC_CLK_ENABLE();

    }
}

/**
  * @brief RTC MSP De-Initialization
  * This function freeze the hardware resources used in this example
  * @param hrtc: RTC handle pointer
  * @retval None
  */
void HAL_RTC_MspDeInit(RTC_HandleTypeDef *hrtc) {
    if (hrtc->Instance == RTC) {

        /* Peripheral clock disable */
        __HAL_RCC_RTC_CLK_DISABLE();

    }
}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
void RTC_Init(void) {

    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    /* Initialize the RTC. */
    hrtc.Instance = RTC;
    hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
    hrtc.Init.AsynchPrediv = 127;
    hrtc.Init.SynchPrediv = 255;
    hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
    hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    if (HAL_RTC_Init(&hrtc) != HAL_OK) {
        Error_Handler();
    }

    /* Preserve the existing startup time and date. */
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
