#ifndef EDGEWORK_BUS_H
#define EDGEWORK_BUS_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool EdgeworkBus_Init(void);

void EdgeworkBus_Service(void);

#ifdef __cplusplus
}
#endif

#endif // EDGEWORK_BUS_H
