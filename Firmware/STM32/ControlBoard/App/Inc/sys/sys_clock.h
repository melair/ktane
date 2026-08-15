#ifndef SYS_CLOCK_H
#define SYS_CLOCK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void SysClock_Init(void);

uint32_t SysClock_GetUs(void);

#ifdef __cplusplus
}
#endif

#endif /* SYS_CLOCK_H */
