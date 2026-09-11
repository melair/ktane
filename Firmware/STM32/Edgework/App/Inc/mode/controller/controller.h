#ifndef EDGEWORK_MODE_CONTROLLER_H
#define EDGEWORK_MODE_CONTROLLER_H

#include <stdint.h>

typedef struct Mode_Definition Mode_Definition;

typedef struct {
    uint8_t reserved;
} Controller_Data;

extern Mode_Definition controller_mode;

#endif // EDGEWORK_MODE_CONTROLLER_H
