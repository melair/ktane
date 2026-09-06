#ifndef MODULE_LINK_H
#define MODULE_LINK_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool ModuleLink_Init(void);

void ModuleLink_Service(void);

void ModuleLink_Front_IRQHandler(void);

void ModuleLink_Rear_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif // MODULE_LINK_H
