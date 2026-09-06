#ifndef MODE_FSM_H
#define MODE_FSM_H

#include <stdbool.h>
#include "fsm/fsm.h"

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
    void (*always_service)(void);
} Mode_Definition;

typedef enum {
    MODE_FSM_STATE_INIT = 0,
    MODE_FSM_STATE_STARTUP,
    MODE_FSM_STATE_IDLE,
    MODE_FSM_STATE_ATTRACT,
    MODE_FSM_STATE_PREPARE,
    MODE_FSM_STATE_READY,
    MODE_FSM_STATE_STARTING,
    MODE_FSM_STATE_RUNNING,
    MODE_FSM_STATE_SOLVED,
    MODE_FSM_STATE_ENDED,
    MODE_FSM_STATE_COUNT,
} Mode_FSM_State;

extern const FSM_State mode_fsm_states[];

#ifdef __cplusplus
}
#endif

#endif //MODE_FSM_H
