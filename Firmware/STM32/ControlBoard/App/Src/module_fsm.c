#include "module_fsm.h"
#include "module.h"
#include <stddef.h>

static void module_fsm_enter(FSM *fsm) {
    const Mode_Definition *mode = fsm->context;
    const Callbacks *callbacks = mode != NULL ? mode->state_callbacks : NULL;

    // TODO: Do we need to queue an announcement for our state change here?

    if ((callbacks != NULL) && (callbacks[fsm->current_id].enter != NULL)) {
        callbacks[fsm->current_id].enter(fsm);
    } else {
        switch (fsm->current_id) {
            case MODULE_FSM_STATE_INIT:
                FSM_Transition(fsm, MODULE_FSM_STATE_STARTUP);
                break;
            case MODULE_FSM_STATE_STARTUP:
                FSM_Transition(fsm, MODULE_FSM_STATE_IDLE);
                break;
            case MODULE_FSM_STATE_PREPARE:
                FSM_Transition(fsm, MODULE_FSM_STATE_READY);
                break;
            default:
                break;
        }
    }
}

static void module_fsm_service(FSM *fsm) {
    const Mode_Definition *mode = fsm->context;
    const Callbacks *callbacks = mode != NULL ? mode->state_callbacks : NULL;

    if ((callbacks != NULL) && (callbacks[fsm->current_id].service != NULL)) {
        if (callbacks[fsm->current_id].service_predicate == NULL || callbacks[fsm->current_id].service_predicate(fsm)) {
            callbacks[fsm->current_id].service(fsm);
        }
    }
}

static void module_fsm_exit(FSM *fsm) {
    const Mode_Definition *mode = fsm->context;
    const Callbacks *callbacks = mode != NULL ? mode->state_callbacks : NULL;

    if ((callbacks != NULL) && (callbacks[fsm->current_id].exit != NULL)) {
        callbacks[fsm->current_id].exit(fsm);
    }
}

const FSM_State module_fsm_states[] = {
    [MODULE_FSM_STATE_INIT] = {
        .enter = module_fsm_enter,
        .service = module_fsm_service,
        .exit = module_fsm_exit,
        .next_mask = FSM_NEXT(MODULE_FSM_STATE_STARTUP),
    },
    [MODULE_FSM_STATE_STARTUP] = {
        .enter = module_fsm_enter,
        .service = module_fsm_service,
        .exit = module_fsm_exit,
        .next_mask = FSM_NEXT(MODULE_FSM_STATE_IDLE),
    },
    [MODULE_FSM_STATE_IDLE] = {
        .enter = module_fsm_enter,
        .service = module_fsm_service,
        .exit = module_fsm_exit,
        .next_mask = FSM_NEXT(MODULE_FSM_STATE_ATTRACT) |
                     FSM_NEXT(MODULE_FSM_STATE_PREPARE),
    },
    [MODULE_FSM_STATE_ATTRACT] = {
        .enter = module_fsm_enter,
        .service = module_fsm_service,
        .exit = module_fsm_exit,
        .next_mask = FSM_NEXT(MODULE_FSM_STATE_IDLE),
    },
    [MODULE_FSM_STATE_PREPARE] = {
        .enter = module_fsm_enter,
        .service = module_fsm_service,
        .exit = module_fsm_exit,
        .next_mask = FSM_NEXT(MODULE_FSM_STATE_READY) |
                     FSM_NEXT(MODULE_FSM_STATE_IDLE),
    },
    [MODULE_FSM_STATE_READY] = {
        .enter = module_fsm_enter,
        .service = module_fsm_service,
        .exit = module_fsm_exit,
        .next_mask = FSM_NEXT(MODULE_FSM_STATE_STARTING) |
                     FSM_NEXT(MODULE_FSM_STATE_IDLE),
    },
    [MODULE_FSM_STATE_STARTING] = {
        .enter = module_fsm_enter,
        .service = module_fsm_service,
        .exit = module_fsm_exit,
        .next_mask = FSM_NEXT(MODULE_FSM_STATE_RUNNING) |
                     FSM_NEXT(MODULE_FSM_STATE_IDLE),
    },
    [MODULE_FSM_STATE_RUNNING] = {
        .enter = module_fsm_enter,
        .service = module_fsm_service,
        .exit = module_fsm_exit,
        .next_mask = FSM_NEXT(MODULE_FSM_STATE_OVER) |
                     FSM_NEXT(MODULE_FSM_STATE_IDLE),
    },
    [MODULE_FSM_STATE_OVER] = {
        .enter = module_fsm_enter,
        .service = module_fsm_service,
        .exit = module_fsm_exit,
        .next_mask = FSM_NEXT(MODULE_FSM_STATE_IDLE),
    },
};
