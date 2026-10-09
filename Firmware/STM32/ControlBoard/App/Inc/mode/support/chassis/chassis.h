#ifndef CHASSIS_H
#define CHASSIS_H

#include "mode_fsm.h"
#include "mode/support/chassis/dac.h"
#include "mode/support/chassis/edgework/bus.h"
#include "mode/support/chassis/edgework/manager.h"
#include "mode/support/chassis/backplane/manager.h"
#include "sys/i2s.h"
#include "audio_mixer.h"

#define CHASSIS_AUDIO_CHANNEL_COUNT 8u

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    DAC_Data dac;
    Bus_Data bus;
    Edgework_Data edgework;
    Backplane_Data backplane;
    AudioData audio;
    int16_t audio_buffer[I2S_AUDIO_BUFFER_SAMPLE_COUNT];
    AudioMixer audio_mixer;
    /* Caller supplies channel source buffers and playback settings. */
    AudioChannel audio_channels[CHASSIS_AUDIO_CHANNEL_COUNT];
} Chassis_Data;

extern Mode_Definition chassis_mode;

#ifdef __cplusplus
}
#endif

#endif //CHASSIS_H
