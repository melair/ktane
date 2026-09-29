#ifndef BACKPLANE_BUS_H
#define BACKPLANE_BUS_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool BackplaneBus_Init(void);

void BackplaneBus_Service(void);

void BackplaneBus_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif // BACKPLANE_BUS_H
