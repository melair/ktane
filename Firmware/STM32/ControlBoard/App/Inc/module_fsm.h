#ifndef MODULE_FSM_H
#define MODULE_FSM_H

#include "sys/fsm.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MODULE_FSM_STATE_INIT = 0,
    MODULE_FSM_STATE_STARTUP,
    MODULE_FSM_STATE_IDLE,
    MODULE_FSM_STATE_ATTRACT,
    MODULE_FSM_STATE_PREPARE,
    MODULE_FSM_STATE_READY,
    MODULE_FSM_STATE_STARTING,
    MODULE_FSM_STATE_RUNNING,
    MODULE_FSM_STATE_OVER,
    MODULE_FSM_STATE_COUNT,
} Module_FSM_State;

extern const FSM_State module_fsm_states[];

#ifdef __cplusplus
}
#endif

#endif //MODULE_FSM_H
