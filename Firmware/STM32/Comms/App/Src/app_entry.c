/**
  ******************************************************************************
  * @file    app_entry.c
  * @author  GPM WBL Application Team
  * @brief   Entry point of the application
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
#include "app_common.h"
#include "main.h"
#include "stm32_seq.h"
#include "app_ble.h"
#include "hw_rng.h"
#include "hw_aes.h"
#include "hw_pka.h"
#include "stm32wb0x.h"
#include "stm32wb0x_ll_usart.h"
#include "ble_stack.h"
#if (CFG_LPM_SUPPORTED == 1)
#include "stm32_lpm.h"
#endif /* CFG_LPM_SUPPORTED */

/* Private includes -----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private defines -----------------------------------------------------------*/

/* Private macros ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Global variables ----------------------------------------------------------*/

/* Private functions prototypes-----------------------------------------------*/

/* External variables --------------------------------------------------------*/

/* Functions Definition ------------------------------------------------------*/

uint32_t MX_APPE_Init(void *p_param) {
    UNUSED(p_param);

#if (CFG_DEBUG_APP_ADV_TRACE != 0)
    UTIL_ADV_TRACE_Init();
    UTIL_ADV_TRACE_SetVerboseLevel(VLEVEL_L); /* functional traces*/
    UTIL_ADV_TRACE_SetRegion(~0x0);
#endif

    if (HW_RNG_Init() != HW_RNG_SUCCESS) {
        Error_Handler();
    }

    /* Init the AES block */
    HW_AES_Init();
    HW_PKA_Init();
    APP_BLE_Init();

#if (CFG_LPM_SUPPORTED == 1)
    /* Low Power Manager Init */
    UTIL_LPM_Init();
#endif /* CFG_LPM_SUPPORTED */

    return BLE_STATUS_SUCCESS;
}

/*************************************************************
 *
 * LOCAL FUNCTIONS
 *
 *************************************************************/
#if (CFG_LPM_SUPPORTED == 1)
static PowerSaveLevels App_PowerSaveLevel_Check(void) {
    PowerSaveLevels output_level = POWER_SAVE_LEVEL_STOP;

    return output_level;
}
#endif

/*************************************************************
 *
 * WRAP FUNCTIONS
 *
 *************************************************************/
void MX_APPE_Process(void) {

    UTIL_SEQ_Run(UTIL_SEQ_DEFAULT);

}

void UTIL_SEQ_PreIdle(void) {
#if (CFG_LPM_SUPPORTED == 1)

#endif /* CFG_LPM_SUPPORTED */
    return;
}

void UTIL_SEQ_Idle(void) {
#if (CFG_LPM_SUPPORTED == 1)

    /* Need to consume some CSTACK on WB05, due to bootloader CSTACK usage. */
    volatile uint32_t dummy[15];
    uint8_t i;
    for (i = 0; i < 10; i++) {
        dummy[i] = 0;
        __NOP();
    }

    PowerSaveLevels app_powerSave_level, vtimer_powerSave_level, final_level, pka_level;

    if ((BLE_STACK_SleepCheck() != POWER_SAVE_LEVEL_RUNNING) &&
        ((app_powerSave_level = App_PowerSaveLevel_Check()) != POWER_SAVE_LEVEL_RUNNING)) {
        vtimer_powerSave_level = HAL_RADIO_TIMER_PowerSaveLevelCheck();
        pka_level = (PowerSaveLevels) HW_PKA_PowerSaveLevelCheck();
        final_level = (PowerSaveLevels) MIN(vtimer_powerSave_level, app_powerSave_level);
        final_level = (PowerSaveLevels) MIN(pka_level, final_level);

        switch (final_level) {
            case POWER_SAVE_LEVEL_RUNNING:
                /* Not Power Save device is busy */
                return;
                break;
            case POWER_SAVE_LEVEL_CPU_HALT:
                UTIL_LPM_SetStopMode(1 << CFG_LPM_APP, UTIL_LPM_DISABLE);
                UTIL_LPM_SetOffMode(1 << CFG_LPM_APP, UTIL_LPM_DISABLE);
                break;
            case POWER_SAVE_LEVEL_STOP_LS_CLOCK_ON:
                UTIL_LPM_SetStopMode(1 << CFG_LPM_APP, UTIL_LPM_ENABLE);
                UTIL_LPM_SetOffMode(1 << CFG_LPM_APP, UTIL_LPM_DISABLE);
                break;
            case POWER_SAVE_LEVEL_STOP:
                UTIL_LPM_SetStopMode(1 << CFG_LPM_APP, UTIL_LPM_ENABLE);
                UTIL_LPM_SetOffMode(1 << CFG_LPM_APP, UTIL_LPM_ENABLE);
                break;
        }

        UTIL_LPM_EnterLowPower();

    }
#endif /* CFG_LPM_SUPPORTED */
}
