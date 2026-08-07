#ifndef MCU_LOAD_H
#define MCU_LOAD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* MCU load fixed-point values. 10000 = 100.00%, 9995 = 99.95%, 1 = 0.01%. */
extern volatile uint16_t mcu_load_1s;
extern volatile uint16_t mcu_load_5s;
extern volatile uint16_t mcu_load_15s;
extern volatile uint16_t mcu_load_60s;

/* Average main-loop wakeups per second over each time band. */
extern volatile uint16_t mcu_wakeups_1s;
extern volatile uint16_t mcu_wakeups_5s;
extern volatile uint16_t mcu_wakeups_15s;
extern volatile uint16_t mcu_wakeups_60s;

void MCU_Load_Init(void);

void MCU_Load_Begin(void);

void MCU_Load_End(void);

#ifdef __cplusplus
}
#endif

#endif /* MCU_LOAD_H */
