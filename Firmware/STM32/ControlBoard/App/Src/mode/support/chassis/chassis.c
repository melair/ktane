#include "mode/support/chassis/chassis.h"
#include "mode/support/chassis/dac.h"
#include "mode/support/chassis/edgework/manager.h"
#include "mode.h"
#include "mode_fsm.h"
#include "sys/i2s.h"
#include "stm32h5xx_it.h"

static Chassis_Data *const chassis = &mode_data.mode.chassis;

static void chassis_fsm_init_enter(FSM *fsm) {
    chassis->audio.buffer = chassis->audio_buffer;
    chassis->audio.frame_count = I2S_AUDIO_FRAME_COUNT;
    AudioMixer_Init(&chassis->audio_mixer, &chassis->audio,
                    chassis->audio_channels, CHASSIS_AUDIO_CHANNEL_COUNT);
    I2S_Init(&chassis->audio, AudioMixer_OutputConsumed, &chassis->audio_mixer);
    DAC_Init();
    if (!Edgework_Init() || !Backplane_Init()) {
        Error_Handler();
    }
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
    AudioMixer_Service(&chassis->audio_mixer);
    DAC_Service();
    Edgework_Service();
    Backplane_Service();
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
