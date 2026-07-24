#include "fsm.h"

#include <string.h>
#include "stm32h5xx_hal.h"

#ifndef FSM_SERVICE_LOOP_LIMIT
#define FSM_SERVICE_LOOP_LIMIT 8
#endif

static bool transition_is_due(const FSM *fsm) {
    return (int32_t) (HAL_GetTick() - fsm->transition_at) >= 0;
}

void FSM_Init(FSM *fsm, const FSM_State *initial_state, void *context) {
    memset(fsm, 0, sizeof(FSM));

    fsm->transition = initial_state;
    fsm->context = context;
}

void FSM_Service(FSM *fsm) {
    uint8_t count = 0;

    do {
        count++;

        if ((fsm->transition != NULL) && transition_is_due(fsm)) {
            if ((fsm->current != NULL) && (fsm->current->exit != NULL)) {
                fsm->current->exit(fsm);
            }

            fsm->current = fsm->transition;
            fsm->transition = NULL;
            fsm->current_id = fsm->current->id;

            if (fsm->current->enter != NULL) {
                fsm->current->enter(fsm);
            }
        }

        if ((fsm->current != NULL) && (fsm->current->service != NULL)) {
            fsm->current->service(fsm);
        }
    } while ((fsm->transition != NULL) && (count < FSM_SERVICE_LOOP_LIMIT));
}

bool FSM_Transition(FSM *fsm, const FSM_State *new_state) {
    if (fsm->transition != NULL) {
        return false;
    }

    if (fsm->current == NULL) {
        return false;
    }

    if (fsm->current->next_states == NULL) {
        return false;
    }

    for (const FSM_State *const *next_state = fsm->current->next_states; *next_state != NULL; next_state++) {
        if (*next_state == new_state) {
            fsm->transition = new_state;
            fsm->transition_at = 0;
            return true;
        }
    }

    return false;
}

bool FSM_TransitionIn(FSM *fsm, const FSM_State *new_state, const uint32_t delay_ms) {
    if (!FSM_Transition(fsm, new_state)) {
        return false;
    }

    fsm->transition_at = HAL_GetTick() + delay_ms;
    return true;
}
