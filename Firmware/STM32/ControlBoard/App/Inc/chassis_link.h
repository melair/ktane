#ifndef CHASSIS_LINK_H
#define CHASSIS_LINK_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool ChassisLink_Init(void);

void ChassisLink_Service(void);

#ifdef __cplusplus
}
#endif

#endif // CHASSIS_LINK_H
