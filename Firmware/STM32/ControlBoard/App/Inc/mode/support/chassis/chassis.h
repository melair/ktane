#ifndef CHASSIS_H
#define CHASSIS_H

#include "module.h"
#include "mode/support/chassis/dac.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    DAC_Data dac;
} Chassis_Data;

extern Mode_Definition chassis_mode;

#ifdef __cplusplus
}
#endif

#endif //CHASSIS_H
