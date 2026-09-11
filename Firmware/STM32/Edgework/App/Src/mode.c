#include "mode.h"

#include "mode/battery/battery.h"
#include "mode/controller/controller.h"
#include "mode/indicator/indicator.h"
#include "mode/ports/ports.h"
#include "mode/serial/serial.h"
#include "mode/twofa/twofa.h"
#include "nvm/nvm.h"

#include <stddef.h>

#define MODE_NVM_ID 0x0000U

typedef struct {
    EdgeworkMode active;
    FSM fsm;
    Mode_Definition *definition;
} Mode_Runtime;

static Mode_Runtime mode = {
    .active = MODE_UNKNOWN,
    .definition = NULL,
};

Mode_Data mode_data = {0};

static void mode_fsm_enter(FSM *fsm) {
    if (fsm->current_id == EDGEWORK_MODE_STATE_DISPLAY) {
        mode_data.current_state = mode_data.desired_state;
        mode_data.dirty = false;
    }

    const Mode_Definition *definition = fsm->context;
    const Mode_Callbacks *callbacks =
        definition != NULL ? definition->state_callbacks : NULL;

    if ((callbacks != NULL) && (callbacks[fsm->current_id].enter != NULL)) {
        callbacks[fsm->current_id].enter(fsm);
        return;
    }

    switch (fsm->current_id) {
        case EDGEWORK_MODE_STATE_INIT:
            (void) FSM_Transition(fsm, EDGEWORK_MODE_STATE_STARTUP);
            break;
        case EDGEWORK_MODE_STATE_STARTUP:
        case EDGEWORK_MODE_STATE_BLANK:
        case EDGEWORK_MODE_STATE_DISPLAY:
            (void) FSM_Transition(fsm, EDGEWORK_MODE_STATE_IDLE);
            break;
        default:
            break;
    }
}

static void mode_fsm_service(FSM *fsm) {
    if ((fsm->current_id == EDGEWORK_MODE_STATE_IDLE) && mode_data.dirty) {
        (void) FSM_Transition(fsm, EDGEWORK_MODE_STATE_DISPLAY);
        return;
    }

    const Mode_Definition *definition = fsm->context;
    const Mode_Callbacks *callbacks =
        definition != NULL ? definition->state_callbacks : NULL;

    if ((callbacks != NULL) && (callbacks[fsm->current_id].service != NULL) &&
        ((callbacks[fsm->current_id].service_predicate == NULL) ||
         callbacks[fsm->current_id].service_predicate(fsm))) {
        callbacks[fsm->current_id].service(fsm);
    }
}

static void mode_fsm_exit(FSM *fsm) {
    const Mode_Definition *definition = fsm->context;
    const Mode_Callbacks *callbacks =
        definition != NULL ? definition->state_callbacks : NULL;

    if ((callbacks != NULL) && (callbacks[fsm->current_id].exit != NULL)) {
        callbacks[fsm->current_id].exit(fsm);
    }
}

static const FSM_State mode_fsm_states[EDGEWORK_MODE_STATE_COUNT] = {
    [EDGEWORK_MODE_STATE_INIT] = {
        .enter = mode_fsm_enter,
        .service = mode_fsm_service,
        .exit = mode_fsm_exit,
        .next_mask = FSM_NEXT(EDGEWORK_MODE_STATE_STARTUP),
    },
    [EDGEWORK_MODE_STATE_STARTUP] = {
        .enter = mode_fsm_enter,
        .service = mode_fsm_service,
        .exit = mode_fsm_exit,
        .next_mask = FSM_NEXT(EDGEWORK_MODE_STATE_IDLE),
    },
    [EDGEWORK_MODE_STATE_IDLE] = {
        .enter = mode_fsm_enter,
        .service = mode_fsm_service,
        .exit = mode_fsm_exit,
        .next_mask = FSM_NEXT(EDGEWORK_MODE_STATE_BLANK) |
                     FSM_NEXT(EDGEWORK_MODE_STATE_DISPLAY),
    },
    [EDGEWORK_MODE_STATE_BLANK] = {
        .enter = mode_fsm_enter,
        .service = mode_fsm_service,
        .exit = mode_fsm_exit,
        .next_mask = FSM_NEXT(EDGEWORK_MODE_STATE_IDLE),
    },
    [EDGEWORK_MODE_STATE_DISPLAY] = {
        .enter = mode_fsm_enter,
        .service = mode_fsm_service,
        .exit = mode_fsm_exit,
        .next_mask = FSM_NEXT(EDGEWORK_MODE_STATE_IDLE),
    },
};

bool Mode_Init(void) {
    const NVM_Query query = {
        .type = UINT8,
        .id = MODE_NVM_ID,
        .data = &mode.active,
    };

    if (!NVM_Read(&query, 1U)) {
        return false;
    }

    switch (mode.active) {
        case MODE_SERIAL:
            mode.definition = &serial_mode;
            break;
        case MODE_INDICATOR:
            mode.definition = &indicator_mode;
            break;
        case MODE_BATTERY:
            mode.definition = &battery_mode;
            break;
        case MODE_PORTS:
            mode.definition = &ports_mode;
            break;
        case MODE_2FA:
            mode.definition = &twofa_mode;
            break;
        case MODE_CONTROLLER:
            mode.definition = &controller_mode;
            break;
        default:
            mode.definition = NULL;
            break;
    }

    return FSM_Init(&mode.fsm, mode_fsm_states, EDGEWORK_MODE_STATE_INIT,
                    mode.definition);
}

void Mode_Service(void) {
    if ((mode.definition != NULL) && (mode.definition->always_service != NULL)) {
        mode.definition->always_service();
    }

    FSM_Service(&mode.fsm);
}

EdgeworkMode Mode_Get(void) {
    return mode.active;
}

bool Mode_Ready(void) {
    return (mode.fsm.current_id >= EDGEWORK_MODE_STATE_IDLE) &&
           (mode.fsm.current_id < EDGEWORK_MODE_STATE_COUNT);
}

edgework_state_t Mode_State(void) {
    return mode_data.current_state;
}

bool Mode_Set(const EdgeworkMode new_mode) {
    EdgeworkMode stored_mode = new_mode;
    const NVM_Query query = {
        .type = UINT8,
        .id = MODE_NVM_ID,
        .data = &stored_mode,
    };

    if (!NVM_Write(&query)) {
        return false;
    }

    mode.active = new_mode;
    return true;
}

bool Mode_Blank(void) {
    return FSM_Transition(&mode.fsm, EDGEWORK_MODE_STATE_BLANK);
}

bool Mode_Display(const edgework_state_t state) {
    mode_data.desired_state = state;
    mode_data.dirty = true;
    return true;
}
