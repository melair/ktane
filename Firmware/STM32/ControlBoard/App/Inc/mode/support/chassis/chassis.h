#ifndef CHASSIS_H
#define CHASSIS_H

#include "mode_fsm.h"
#include "mode/support/chassis/dac.h"
#include "mode/support/chassis/edgework/bus.h"
#include "mode/support/chassis/edgework/manager.h"
#include "sys/i2s.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    DAC_Data dac;
    Bus_Data bus;
    Edgework_Data edgework;
    AudioData audio;
    int16_t audio_buffer[I2S_AUDIO_BUFFER_SAMPLE_COUNT];
} Chassis_Data;

extern Mode_Definition chassis_mode;

#ifdef __cplusplus
}
#endif

#endif //CHASSIS_H
