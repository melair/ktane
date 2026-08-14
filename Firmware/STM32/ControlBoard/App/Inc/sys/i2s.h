#ifndef I2S_H
#define I2S_H

#include <stdint.h>

#define I2S_AUDIO_FRAME_COUNT 4800u
#define I2S_AUDIO_CHANNEL_COUNT 2u
#define I2S_AUDIO_BUFFER_SAMPLE_COUNT (I2S_AUDIO_FRAME_COUNT * I2S_AUDIO_CHANNEL_COUNT)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int16_t *buffer;
    uint16_t buffer_size;
} AudioData;

void I2S_Init(AudioData *audio);

void I2S_Service(AudioData *audio);

void I2S_Fill_Sine(AudioData *audio);

/* Current stereo frame being consumed by the circular I2S DMA transfer. */
uint32_t I2S_GetReadFrame(const AudioData *audio);

#ifdef __cplusplus
}
#endif

#endif //I2S_H
