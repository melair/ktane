#ifndef FSM_H
#define FSM_H

#include <stdbool.h>
#include <stdint.h>

typedef struct FSM FSM;
typedef struct FSM_State FSM_State;
typedef uint8_t FSM_StateId;
typedef uint32_t FSM_NextMask;

#define FSM_INVALID_STATE UINT8_MAX
#define FSM_MAX_STATES 32U
#define FSM_NEXT(state_id) ((FSM_NextMask) 1UL << (state_id))

struct FSM_State {
    void (*enter)(FSM *fsm);

    void (*service)(FSM *fsm);

    bool (*service_predicate)(FSM *fsm);

    void (*exit)(FSM *fsm);

    FSM_NextMask next_mask;
};

struct FSM {
    const FSM_State *states;

    FSM_StateId current_id;
    FSM_StateId transition_id;
    bool transition_pending;
    uint32_t transition_at;

    void *context;
};

bool FSM_Init(FSM *fsm, const FSM_State *states, FSM_StateId initial_state_id,
              void *context);

void FSM_Service(FSM *fsm);

bool FSM_Transition(FSM *fsm, FSM_StateId new_state_id);

bool FSM_TransitionIn(FSM *fsm, FSM_StateId new_state_id, uint32_t delay_ms);

#endif //FSM_H
