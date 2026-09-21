/**
  ******************************************************************************
  * @file    stm32wb0x_it.h
  * @brief   This file contains the headers of the interrupt handlers.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32WB0x_IT_H
#define __STM32WB0x_IT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Private includes ----------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
void NMI_Handler(void);

void HardFault_Handler(void);

void SVC_Handler(void);

void PendSV_Handler(void);

void SysTick_Handler(void);

void LPUART1_IRQHandler(void);

void PKA_IRQHandler(void);

void RADIO_TIMER_CPU_WKUP_IRQHandler(void);

void RADIO_TIMER_ERROR_IRQHandler(void);

void RADIO_TXRX_IRQHandler(void);

void RADIO_TXRX_SEQ_IRQHandler(void);

void RADIO_RRM_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* __STM32WB0x_IT_H */
