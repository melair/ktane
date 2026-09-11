#ifndef EDGEWORK_MODE_BATTERY_H
#define EDGEWORK_MODE_BATTERY_H

#include <stdint.h>

typedef struct Mode_Definition Mode_Definition;

typedef struct {
    uint8_t count;
} Battery_Data;

extern Mode_Definition battery_mode;

#endif // EDGEWORK_MODE_BATTERY_H
