/**
  ******************************************************************************
  * @file    usart_if.c
  * @author  GPM WBL Application Team
  * @brief : Source file for interfacing the stm32_adv_trace to hardware
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
#include "stm32_adv_trace.h"
#include "usart_if.h"

/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define RECEIVE_AFTER_TRANSMIT  0 /* Whether the UART should be in RX after a Transmit */

/* External variables --------------------------------------------------------*/
/**
  * @brief LPUART1 handle
  */
extern UART_HandleTypeDef hlpuart1;

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/**
 *  @brief  trace tracer definition.
 *
 *  list all the driver interface used by the trace application.
 */
const UTIL_ADV_TRACE_Driver_s UTIL_TraceDriver =
{
    UART_Init,
    UART_DeInit,
    UART_StartRx,
    UART_TransmitDMA
};

/* Private variables ---------------------------------------------------------*/

#if (CFG_DEBUG_APP_ADV_TRACE != 0)

/**
  * @brief buffer to receive 1 character
  */
uint8_t charRx;

/**
  * @brief  TX complete callback
  * @return none
  */
static void (*TxCpltCallback)(void *);
static void (*RxCpltCallback)(uint8_t *pdata, uint16_t size, uint8_t error);

#endif /* (CFG_DEBUG_APP_ADV_TRACE != 0) */

/* Exported macro ------------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

#if (CFG_DEBUG_APP_ADV_TRACE != 0)

static void LPUART1_DMA_MspDeInit(void);

#endif /* (CFG_DEBUG_APP_ADV_TRACE != 0) */

/* Private user code ---------------------------------------------------------*/

UTIL_ADV_TRACE_Status_t UART_Init(void (*cb)(void *)) {
#if (CFG_DEBUG_APP_ADV_TRACE != 0)

    /* LPUART1 is initialized in main.c before the BLE trace module starts. */

    TxCpltCallback = cb;

#endif /* (CFG_DEBUG_APP_ADV_TRACE != 0) */

    return UTIL_ADV_TRACE_OK;

}

UTIL_ADV_TRACE_Status_t UART_DeInit(void) {
#if (CFG_DEBUG_APP_ADV_TRACE != 0)

    HAL_StatusTypeDef result;

    LPUART1_DMA_MspDeInit();

    result = HAL_UART_DeInit(&hlpuart1);
    if (result != HAL_OK) {
        TxCpltCallback = NULL;
        return UTIL_ADV_TRACE_UNKNOWN_ERROR;
    }

    if (hlpuart1.hdmatx) {
        result = HAL_DMA_DeInit(hlpuart1.hdmatx);
        if (result != HAL_OK) {
            return UTIL_ADV_TRACE_UNKNOWN_ERROR;
        }
    }

    if (hlpuart1.hdmarx) {
        result = HAL_DMA_DeInit(hlpuart1.hdmarx);
        if (result != HAL_OK) {
            return UTIL_ADV_TRACE_UNKNOWN_ERROR;
        }
    }

#endif /* (CFG_DEBUG_APP_ADV_TRACE != 0) */

    return UTIL_ADV_TRACE_OK;

}

UTIL_ADV_TRACE_Status_t UART_StartRx(void(*cb)(uint8_t * pdata, uint16_t size, uint8_t error)) {
#if (CFG_DEBUG_APP_ADV_TRACE != 0)

    /* Configure LPUART1 in Receive mode */
    HAL_UART_Receive_IT(&hlpuart1, &charRx, 1);

    if (cb != NULL) {
        RxCpltCallback = cb;
    }

#endif /* (CFG_DEBUG_APP_ADV_TRACE != 0) */

    return UTIL_ADV_TRACE_OK;

}

UTIL_ADV_TRACE_Status_t UART_TransmitDMA(uint8_t *pdata, uint16_t size) {

    UTIL_ADV_TRACE_Status_t status = UTIL_ADV_TRACE_OK;

#if (CFG_DEBUG_APP_ADV_TRACE != 0)

    HAL_StatusTypeDef result;

    if (hlpuart1.hdmatx) {
        result = HAL_UART_Transmit_DMA(&hlpuart1, pdata, size);
    } else {
        result = HAL_UART_Transmit_IT(&hlpuart1, pdata, size);
    }

    if (result != HAL_OK) {
        status = UTIL_ADV_TRACE_HW_ERROR;
    }

#if RECEIVE_AFTER_TRANSMIT
    HAL_UART_Receive_IT(&hlpuart1, &charRx, 1);
#endif

#endif /* (CFG_DEBUG_APP_ADV_TRACE != 0) */

    return status;

}

#if (CFG_DEBUG_APP_ADV_TRACE != 0)

static void LPUART1_DMA_MspDeInit(void) {

    /* Disable interrupts for LPUART1. */
    HAL_NVIC_DisableIRQ(LPUART1_IRQn);

    /* GPDMA1 controller clock disable */
    __HAL_RCC_DMA_CLK_DISABLE();

    /* DMA interrupt init */
    HAL_NVIC_DisableIRQ(DMA_IRQn);

}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {

    /* ADV Trace callback */
    if (TxCpltCallback)
        TxCpltCallback(NULL);

}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {

    RxCpltCallback(&charRx, 1, 0);
    HAL_UART_Receive_IT(&hlpuart1, &charRx, 1);

}

#endif /* (CFG_DEBUG_APP_ADV_TRACE != 0) */

/* Private user code ---------------------------------------------------------*/
