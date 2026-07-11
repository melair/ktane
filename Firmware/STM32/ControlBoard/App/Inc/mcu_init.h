/**
  ******************************************************************************
  * @file           : mcu_init.h
  * @brief          : MCU initialization function declarations.
  ******************************************************************************
  */
#ifndef MCU_INIT_H
#define MCU_INIT_H

#ifdef __cplusplus
extern "C" {
#endif

void MPU_Init(void);
void SystemClock_Init(void);
void PeripheralClock_Init(void);
void GPIO_Init(void);
void ICACHE_Init(void);
void RTC_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* MCU_INIT_H */
