#ifndef FSM_H
#define FSM_H

#include <stdbool.h>
#include <stdint.h>

typedef struct FSM FSM;
typedef struct FSM_State FSM_State;

struct FSM_State {
    uint8_t id;

    void (*enter)(FSM *fsm);
    void (*service)(FSM *fsm);
    void (*exit)(FSM *fsm);

    const FSM_State *const *next_states;
};

struct FSM {
    const FSM_State *current;
    const FSM_State *transition;
    uint32_t transition_at;
    uint8_t current_id;

    void *context;
};

void FSM_Init(FSM *fsm, const FSM_State *initial_state, void *context);
void FSM_Service(FSM *fsm);
bool FSM_Transition(FSM *fsm, const FSM_State *new_state);
bool FSM_TransitionIn(FSM *fsm, const FSM_State *new_state, uint32_t delay_ms);

#endif //FSM_H
