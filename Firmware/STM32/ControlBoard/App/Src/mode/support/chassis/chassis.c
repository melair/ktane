#include "mode/support/chassis/chassis.h"
#include "mode/support/chassis/dac.h"
#include "mode.h"
#include "mode_fsm.h"
#include "sys/i2s.h"

static Chassis_Data *const chassis = &mode_data.mode.chassis;

static void chassis_fsm_init_enter(FSM *fsm) {
    chassis->audio.buffer = chassis->audio_buffer;
    chassis->audio.buffer_size = I2S_AUDIO_BUFFER_SAMPLE_COUNT;
    I2S_Init(&chassis->audio);
    DAC_Init();
    Mode_SetServiceEnabled(true);
}

static void chassis_fsm_init_service(FSM *fsm) {
    if (DAC_Ready()) {
        FSM_Transition(fsm, MODE_FSM_STATE_STARTUP);
        DAC_Volume(-50);
        DAC_Mute(false);
    }
}

static void chassis_always_service(void) {
    I2S_Service(&chassis->audio);
    DAC_Service();
}

static Callbacks chassis_state_callbacks[MODE_FSM_STATE_COUNT] = {
    [MODE_FSM_STATE_INIT] = {
        .enter = chassis_fsm_init_enter,
        .service = chassis_fsm_init_service,
    },
    [MODE_FSM_STATE_STARTUP] = {0},
    [MODE_FSM_STATE_IDLE] = {0},
    [MODE_FSM_STATE_ATTRACT] = {0},
    [MODE_FSM_STATE_PREPARE] = {0},
    [MODE_FSM_STATE_READY] = {0},
    [MODE_FSM_STATE_STARTING] = {0},
    [MODE_FSM_STATE_RUNNING] = {0},
    [MODE_FSM_STATE_SOLVED] = {0},
    [MODE_FSM_STATE_ENDED] = {0},
};

Mode_Definition chassis_mode = {
    .state_callbacks = chassis_state_callbacks,
    .always_service = chassis_always_service,
};
