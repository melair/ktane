#include "sys/rtc.h"

#include "main.h"
#include "stm32h5xx_it.h"

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
        PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
        PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_HSE_DIV25;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
            Error_Handler();
        }

        __HAL_RCC_RTC_ENABLE();
        __HAL_RCC_RTCAPB_CLK_ENABLE();
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
        __HAL_RCC_RTC_DISABLE();
        __HAL_RCC_RTCAPB_CLK_DISABLE();
    }
}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
void RTC_Init(void) {
    /** Initialize RTC Only */
    hrtc.Instance = RTC;
    hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
    hrtc.Init.AsynchPrediv = 124;
    hrtc.Init.SynchPrediv = 7999;
    hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
    hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
    hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
    hrtc.Init.OutPutPullUp = RTC_OUTPUT_PULLUP_NONE;
    hrtc.Init.BinMode = RTC_BINARY_NONE;
    if (HAL_RTC_Init(&hrtc) != HAL_OK) {
        Error_Handler();
    }

    RTC_PrivilegeStateTypeDef privilegeState = {0};

    privilegeState.rtcPrivilegeFull = RTC_PRIVILEGE_FULL_NO;
    privilegeState.backupRegisterPrivZone = RTC_PRIVILEGE_BKUP_ZONE_NONE;
    privilegeState.backupRegisterStartZone2 = RTC_BKP_DR0;
    privilegeState.backupRegisterStartZone3 = RTC_BKP_DR0;
    if (HAL_RTCEx_PrivilegeModeSet(&hrtc, &privilegeState) != HAL_OK) {
        Error_Handler();
    }
}

void RTC_GetTime(RTC_Time *time) {
    RTC_TimeTypeDef rtcTime = {0};
    RTC_DateTypeDef rtcDate = {0};

    if (time == NULL) {
        return;
    }

    if (HAL_RTC_GetTime(&hrtc, &rtcTime, RTC_FORMAT_BIN) != HAL_OK) {
        Error_Handler();
    }

    /* Reading the date unlocks the RTC shadow registers after reading the time. */
    if (HAL_RTC_GetDate(&hrtc, &rtcDate, RTC_FORMAT_BIN) != HAL_OK) {
        Error_Handler();
    }

    time->hours = rtcTime.Hours;
    time->minutes = rtcTime.Minutes;
    time->seconds = rtcTime.Seconds;
}

void RTC_SetTime(const RTC_Time *time) {
    if (time == NULL) {
        return;
    }

    RTC_TimeTypeDef rtcTime = {
        .Hours = time->hours,
        .Minutes = time->minutes,
        .Seconds = time->seconds,
    };

    if (HAL_RTC_SetTime(&hrtc, &rtcTime, RTC_FORMAT_BIN) != HAL_OK) {
        Error_Handler();
    }
}
