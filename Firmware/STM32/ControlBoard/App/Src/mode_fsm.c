#include "mode_fsm.h"
#include "mode.h"
#include <stddef.h>

static void mode_fsm_enter(FSM *fsm) {
    const Mode_Definition *mode = fsm->context;
    const Callbacks *callbacks = mode != NULL ? mode->state_callbacks : NULL;

    // TODO: Do we need to queue an announcement for our state change here?

    if ((callbacks != NULL) && (callbacks[fsm->current_id].enter != NULL)) {
        callbacks[fsm->current_id].enter(fsm);
    } else {
        switch (fsm->current_id) {
            case MODE_FSM_STATE_INIT:
                FSM_Transition(fsm, MODE_FSM_STATE_STARTUP);
                break;
            case MODE_FSM_STATE_STARTUP:
                FSM_Transition(fsm, MODE_FSM_STATE_IDLE);
                break;
            case MODE_FSM_STATE_PREPARE:
                FSM_Transition(fsm, MODE_FSM_STATE_READY);
                break;
            default:
                break;
        }
    }
}

static void mode_fsm_service(FSM *fsm) {
    const Mode_Definition *mode = fsm->context;
    const Callbacks *callbacks = mode != NULL ? mode->state_callbacks : NULL;

    if ((callbacks != NULL) && (callbacks[fsm->current_id].service != NULL)) {
        if (callbacks[fsm->current_id].service_predicate == NULL || callbacks[fsm->current_id].service_predicate(fsm)) {
            callbacks[fsm->current_id].service(fsm);
        }
    }
}

static void mode_fsm_exit(FSM *fsm) {
    const Mode_Definition *mode = fsm->context;
    const Callbacks *callbacks = mode != NULL ? mode->state_callbacks : NULL;

    if ((callbacks != NULL) && (callbacks[fsm->current_id].exit != NULL)) {
        callbacks[fsm->current_id].exit(fsm);
    }
}

const FSM_State mode_fsm_states[] = {
    [MODE_FSM_STATE_INIT] = {
        .enter = mode_fsm_enter,
        .service = mode_fsm_service,
        .exit = mode_fsm_exit,
        .next_mask = FSM_NEXT(MODE_FSM_STATE_STARTUP),
    },
    [MODE_FSM_STATE_STARTUP] = {
        .enter = mode_fsm_enter,
        .service = mode_fsm_service,
        .exit = mode_fsm_exit,
        .next_mask = FSM_NEXT(MODE_FSM_STATE_IDLE),
    },
    [MODE_FSM_STATE_IDLE] = {
        .enter = mode_fsm_enter,
        .service = mode_fsm_service,
        .exit = mode_fsm_exit,
        .next_mask = FSM_NEXT(MODE_FSM_STATE_ATTRACT) |
                     FSM_NEXT(MODE_FSM_STATE_PREPARE),
    },
    [MODE_FSM_STATE_ATTRACT] = {
        .enter = mode_fsm_enter,
        .service = mode_fsm_service,
        .exit = mode_fsm_exit,
        .next_mask = FSM_NEXT(MODE_FSM_STATE_IDLE),
    },
    [MODE_FSM_STATE_PREPARE] = {
        .enter = mode_fsm_enter,
        .service = mode_fsm_service,
        .exit = mode_fsm_exit,
        .next_mask = FSM_NEXT(MODE_FSM_STATE_READY) |
                     FSM_NEXT(MODE_FSM_STATE_IDLE),
    },
    [MODE_FSM_STATE_READY] = {
        .enter = mode_fsm_enter,
        .service = mode_fsm_service,
        .exit = mode_fsm_exit,
        .next_mask = FSM_NEXT(MODE_FSM_STATE_STARTING) |
                     FSM_NEXT(MODE_FSM_STATE_IDLE),
    },
    [MODE_FSM_STATE_STARTING] = {
        .enter = mode_fsm_enter,
        .service = mode_fsm_service,
        .exit = mode_fsm_exit,
        .next_mask = FSM_NEXT(MODE_FSM_STATE_RUNNING) |
                     FSM_NEXT(MODE_FSM_STATE_IDLE),
    },
    [MODE_FSM_STATE_RUNNING] = {
        .enter = mode_fsm_enter,
        .service = mode_fsm_service,
        .exit = mode_fsm_exit,
        .next_mask = FSM_NEXT(MODE_FSM_STATE_ENDED) |
                     FSM_NEXT(MODE_FSM_STATE_SOLVED) |
                     FSM_NEXT(MODE_FSM_STATE_IDLE),
    },
    [MODE_FSM_STATE_SOLVED] = {
        .enter = mode_fsm_enter,
        .service = mode_fsm_service,
        .exit = mode_fsm_exit,
        .next_mask = FSM_NEXT(MODE_FSM_STATE_ENDED) |
                     FSM_NEXT(MODE_FSM_STATE_IDLE),
    },
    [MODE_FSM_STATE_ENDED] = {
        .enter = mode_fsm_enter,
        .service = mode_fsm_service,
        .exit = mode_fsm_exit,
        .next_mask = FSM_NEXT(MODE_FSM_STATE_IDLE),
    },
};
