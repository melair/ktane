#ifndef MODULE_H
#define MODULE_H
#include <stdbool.h>
#include <stdint.h>
#include "sys/fsm.h"

#ifdef __cplusplus
extern "C" {

#endif

typedef struct {
    void (*enter)(FSM *fsm);
    void (*service)(FSM *fsm);
    bool (*service_predicate)(FSM *fsm);
    void (*exit)(FSM *fsm);
} Callbacks;

typedef struct {
    Callbacks *state_callbacks;
    void (*service)(void);
} Mode_Definition;

void Module_Init(void);

void Module_Service(void);

void Module_SetServiceEnabled(bool enabled);

void Module_Set(uint8_t mode);

#ifdef __cplusplus
}
#endif

#endif //MODULE_H
