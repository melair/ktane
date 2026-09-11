#ifndef EDGEWORK_MODE_PORTS_H
#define EDGEWORK_MODE_PORTS_H

#include <stdint.h>

typedef struct Mode_Definition Mode_Definition;

typedef struct {
    uint8_t reserved;
} Ports_Data;

extern Mode_Definition ports_mode;

#endif // EDGEWORK_MODE_PORTS_H
