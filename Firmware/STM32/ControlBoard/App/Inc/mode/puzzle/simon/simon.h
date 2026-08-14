#ifndef SIMON_H
#define SIMON_H

#include "mode_fsm.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t reserved;
} Simon_Data;

extern Mode_Definition simon_mode;

#ifdef __cplusplus
}
#endif

#endif //SIMON_H
