#ifndef I2S_H
#define I2S_H

#include "audio.h"

#define I2S_AUDIO_FRAME_COUNT 960u
#define I2S_AUDIO_CHANNEL_COUNT 2u
#define I2S_AUDIO_BUFFER_SAMPLE_COUNT (I2S_AUDIO_FRAME_COUNT * I2S_AUDIO_CHANNEL_COUNT)

#ifdef __cplusplus
extern "C" {
#endif

/* Starts DMA on caller-owned stereo data. frame_count must be positive,
 * even and at most 32767. With 960 frames each half lasts 10 ms at 48 kHz.
 * The callback reports the consumed half in interrupt context; NULL is
 * allowed. The supplied context is passed unchanged to the callback.
 */
void I2S_Init(AudioData *audio, AudioBufferCallback callback, void *context);

void I2S_Fill_Sine(AudioData *audio);

#ifdef __cplusplus
}
#endif

#endif //I2S_H
