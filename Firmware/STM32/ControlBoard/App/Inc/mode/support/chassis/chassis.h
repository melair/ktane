#ifndef CHASSIS_H
#define CHASSIS_H

#include "module.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t reserved;
} Chassis_Data;

extern Mode_Definition chassis_mode;

#ifdef __cplusplus
}
#endif

#endif //CHASSIS_H
