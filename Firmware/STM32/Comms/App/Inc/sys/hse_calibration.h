/**
  ******************************************************************************
  * @file           : hse_calibration.h
  * @brief          : HSE calibration lifecycle.
  ******************************************************************************
  */
#ifndef HSE_CALIBRATION_H
#define HSE_CALIBRATION_H

#ifdef __cplusplus
extern "C" {
#endif

/* Configure TIM2 and its reference input once, after system clock/GPIO setup. */
void HSE_Calibration_Init(void);

/* Start reference capture once, after Init and before servicing calibration. */
void HSE_Calibration_Start(void);

/* Process captured measurements without blocking; call from the main loop. */
void HSE_Calibration_Service(void);

#ifdef __cplusplus
}
#endif

#endif /* HSE_CALIBRATION_H */
