#include "fsm.h"
#include "time.h"
#include <stddef.h>
#include <stdint.h>

#ifndef FSM_SERVICE_LOOP_LIMIT
#define FSM_SERVICE_LOOP_LIMIT 8
#endif

/* Initialise the FSM so that fsm_service() will start the FSM correctly. */
void fsm_init(fsm_t *fsm) {
  /* Ensure current is NULL, and set first transition to initial state. */
  fsm->current = NULL;
  fsm->transition = fsm->initial;
  fsm->transition_at = 0;
}

/* Service the fsm.

   Upon start fsm_service() will begin a transition, if there is one, by calling
   the current states exit(), then swap, call the new states enter(). Then
   finally calling service().

   This process will repeat if a transition is requested by either the enter()
   or service() call backs.

   Because an FSM that simply jumps between multiple states will hang the
   microcontroller there is a limit on the number of FSM transitions that
   fsm_service() will perform in one service loop (FSM_SERVICE_LOOP_LIMIT). This
   is an antipattern and FSMs should not make use of this functionality.
   */
extern const fs_t i2c_state_idle;

void fsm_service(fsm_t *fsm) {
  uint8_t count = 0;

  do {
    /* Increment loop count. */
    count++;

    /* Check to see if we need to transition. */
    if (fsm->transition != NULL && fsm->transition_at <= uptime_in_ms) {
      /* If we have a current state (may not at init) and we have an exit
       * callback, do it. */
      if (fsm->current != NULL && fsm->current->exit != NULL) {
        fsm->current->exit(fsm);
      }

      fsm->current = fsm->transition;
      fsm->transition = NULL;

      /* If new state has an enter call back, do it. */
      if (fsm->current->enter != NULL) {
        fsm->current->enter(fsm);
      }
    }

    /* Also then service. */
    if (fsm->current != NULL && fsm->current->service != NULL) {
      fsm->current->service(fsm);
    }
  } while (fsm->transition != NULL && count < FSM_SERVICE_LOOP_LIMIT);
}

/* Transition the FSM from one state to another.

   If a transition is already queue then this function will return false, and
   will not transition as requested.

   This can be called at any time, however due to the above a call in an exit()
   callback of a state will never succeed.
   */
bool fsm_transition(fsm_t *fsm, const fs_t *new_state) {
  /* We can not transition if it has been requested. */
  if (fsm->transition != NULL) {
    return false;
  }

#ifndef FSM_SKIP_LEGAL_CHECK
  if (fsm->current == NULL) {
    return false;
  }

  bool legal = false;

  /* Ensure next state is a legal transition. */
  for (uint8_t i = 0; i < FSM_MAX_NEXT_STATES; i++) {
    if (fsm->current->next_states[i] == NULL) {
      continue;
    }

    if (fsm->current->next_states[i] == new_state) {
      legal = true;
      break;
    }
  }

  /* If the move is legal, queue it. */
  if (legal) {
    fsm->transition = new_state;
    fsm->transition_at = 0;
    return true;
  } else {
    return false;
  }
#else
  fsm->transition = new_state;
  fsm->transition_at = 0;
  return true;
#endif
}

bool fsm_transition_in(fsm_t *fsm, const fs_t *new_state, uint32_t delayInMs) {
  if (fsm_transition(fsm, new_state)) {
    fsm->transition_at = uptime_in_ms + delayInMs;

    return true;
  } else {
    return false;
  }
}
