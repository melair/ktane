#ifndef CHASSIS_BUS_H
#define CHASSIS_BUS_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool ChassisBus_Init(void);

void ChassisBus_Service(void);

#ifdef __cplusplus
}
#endif

#endif //CHASSIS_BUS_H
