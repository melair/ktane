#ifndef CHASSIS_H
#define CHASSIS_H

#include "module.h"
#include "mode/support/chassis/dac.h"
#include "sys/i2s.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    DAC_Data dac;
    AudioData audio;
    int16_t audio_buffer[I2S_AUDIO_BUFFER_SAMPLE_COUNT];
} Chassis_Data;

extern Mode_Definition chassis_mode;

#ifdef __cplusplus
}
#endif

#endif //CHASSIS_H
