#include "mode/support/chassis/chassis.h"
#include "mode/support/chassis/dac.h"
#include "mode.h"
#include "module_fsm.h"
#include <stddef.h>

static Chassis_Data *const chassis = &module_data.mode.chassis;

static void chassis_fsm_init_enter(FSM *fsm) {
    DAC_Init();
    Module_SetServiceEnabled(true);
}

static void chassis_fsm_init_service(FSM *fsm) {
    if (DAC_Ready()) {
        FSM_Transition(fsm, MODULE_FSM_STATE_STARTUP);
    }
}

static void chassis_always_service(void) {
    DAC_Service();
}

static Callbacks chassis_state_callbacks[MODULE_FSM_STATE_COUNT] = {
    [MODULE_FSM_STATE_INIT] = {
        .enter = chassis_fsm_init_enter,
        .service = chassis_fsm_init_service,
    },
    [MODULE_FSM_STATE_STARTUP] = {0},
    [MODULE_FSM_STATE_IDLE] = {0},
    [MODULE_FSM_STATE_ATTRACT] = {0},
    [MODULE_FSM_STATE_PREPARE] = {0},
    [MODULE_FSM_STATE_READY] = {0},
    [MODULE_FSM_STATE_STARTING] = {0},
    [MODULE_FSM_STATE_RUNNING] = {0},
    [MODULE_FSM_STATE_OVER] = {0},
};

Mode_Definition chassis_mode = {
    .state_callbacks = chassis_state_callbacks,
    .always_service = chassis_always_service,
};
