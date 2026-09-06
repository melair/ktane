#include "fsm/fsm.h"

#include <string.h>

// Every STM32 HAL family exposes this function with the same signature. A
// local declaration keeps this common module independent of family headers.
extern uint32_t HAL_GetTick(void);

#ifndef FSM_SERVICE_LOOP_LIMIT
#define FSM_SERVICE_LOOP_LIMIT 8
#endif

static bool transition_is_due(const FSM *fsm) {
    return (int32_t) (HAL_GetTick() - fsm->transition_at) >= 0;
}

bool FSM_Init(FSM *fsm, const FSM_State *states, const FSM_StateId initial_state_id,
              void *context) {
    memset(fsm, 0, sizeof(FSM));

    fsm->states = states;
    fsm->current_id = FSM_INVALID_STATE;
    fsm->transition_id = FSM_INVALID_STATE;
    fsm->context = context;

    if ((states == NULL) || (initial_state_id >= FSM_MAX_STATES)) {
        return false;
    }

    fsm->transition_id = initial_state_id;
    fsm->transition_pending = true;
    fsm->transition_at = 0;
    return true;
}

void FSM_Service(FSM *fsm) {
    uint8_t count = 0;

    do {
        count++;

        if (fsm->transition_pending && transition_is_due(fsm)) {
            if (fsm->current_id != FSM_INVALID_STATE) {
                const FSM_State *state = &fsm->states[fsm->current_id];
                if (state->exit != NULL) {
                    state->exit(fsm);
                }
            }

            fsm->current_id = fsm->transition_id;
            fsm->transition_id = FSM_INVALID_STATE;
            fsm->transition_pending = false;

            const FSM_State *state = &fsm->states[fsm->current_id];
            if (state->enter != NULL) {
                state->enter(fsm);
            }
        }

        const FSM_State *state = &fsm->states[fsm->current_id];
        if ((state->service != NULL) &&
            ((state->service_predicate == NULL) || state->service_predicate(fsm))) {
            state->service(fsm);
        }

        if (fsm->transition_pending && !transition_is_due(fsm)) {
            break;
        }
    } while (fsm->transition_pending && (count < FSM_SERVICE_LOOP_LIMIT));
}

bool FSM_IsTransitionLegal(const FSM *fsm, const FSM_StateId new_state_id) {
    if ((fsm == NULL) ||
        (fsm->states == NULL) ||
        (fsm->current_id >= FSM_MAX_STATES) ||
        (new_state_id >= FSM_MAX_STATES)) {
        return false;
    }

    return (fsm->states[fsm->current_id].next_mask & FSM_NEXT(new_state_id)) != 0U;
}

bool FSM_Transition(FSM *fsm, const FSM_StateId new_state_id) {
    if ((fsm == NULL) || fsm->transition_pending || !FSM_IsTransitionLegal(fsm, new_state_id)) {
        return false;
    }

    fsm->transition_id = new_state_id;
    fsm->transition_pending = true;
    fsm->transition_at = 0;
    return true;
}

bool FSM_TransitionIn(FSM *fsm, const FSM_StateId new_state_id, const uint32_t delay_ms) {
    if (!FSM_Transition(fsm, new_state_id)) {
        return false;
    }

    fsm->transition_at = HAL_GetTick() + delay_ms;
    return true;
}
