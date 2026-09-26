/**
  ******************************************************************************
  * @file    stm32wb0x_it.c
  * @brief   Interrupt Service Routines.
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
#include "stm32wb0x_it.h"
#include "hw_pka.h"
#include "ble_stack.h"
#include "miscutil.h"
#include "stm32wb0x_ll_usart.h"
/* External variables --------------------------------------------------------*/

extern PKA_HandleTypeDef hpka;

/******************************************************************************/
/*           Cortex Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void) {
    while (1) {
    }
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void) {
    while (1) {
    }
}

/**
  * @brief This function handles System service call via SWI instruction.
  */
void SVC_Handler(void) {
}

/**
  * @brief This function handles Pendable request for system service.
  */
void PendSV_Handler(void) {
}

/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void) {
    HAL_IncTick();
}

/**
  * @brief This function handles LPUART1 global interrupt.
  */
void LPUART1_IRQHandler(void) {
    HAL_UART_IRQHandler(&hlpuart1);
}

/**
  * @brief Complete BLE Secure Connections public-key and DHKey operations.
  */
void PKA_IRQHandler(void) {
    HAL_PKA_IRQHandler(&hpka);
}

/******************************************************************************/
/* STM32WB0x Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32wb0x.s).                    */
/******************************************************************************/

/**
  * @brief This function handles RADIO_TIMER_CPU_WKUP global interrupt.
  */
void RADIO_TIMER_CPU_WKUP_IRQHandler(void) {
    HAL_RADIO_TIMER_CPU_WKUP_IRQHandler();
}

/**
  * @brief This function handles RADIO_TIMER_ERROR global interrupt.
  */
void RADIO_TIMER_ERROR_IRQHandler(void) {
    HAL_RADIO_TIMER_ERROR_IRQHandler();
}

/**
  * @brief This function handles RADIO_TXRX global interrupt.
  */
void RADIO_TXRX_IRQHandler(void) {
    HAL_RADIO_TXRX_IRQHandler();
}

/**
  * @brief This function handles RADIO_TXRX_SEQ global interrupt.
  */
void RADIO_TXRX_SEQ_IRQHandler(void) {
    HAL_RADIO_TXRX_SEQ_IRQHandler();
}

/**
  * @brief This function handles RADIO_RRM global interrupt.
  */
void RADIO_RRM_IRQHandler(void) {
    HAL_RADIO_RRM_IRQHandler();
}
