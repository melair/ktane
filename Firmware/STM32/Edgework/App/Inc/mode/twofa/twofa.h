#ifndef EDGEWORK_MODE_TWOFA_H
#define EDGEWORK_MODE_TWOFA_H

#include <stdint.h>

typedef struct Mode_Definition Mode_Definition;

typedef struct {
    uint8_t reserved;
} TwoFA_Data;

extern Mode_Definition twofa_mode;

#endif // EDGEWORK_MODE_TWOFA_H
