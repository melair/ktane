#ifndef AUDIO_MIXER_H
#define AUDIO_MIXER_H

#include "audio.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Caller-allocated storage. All fields are implementation-owned. */
typedef struct AudioMixer {
    AudioData *output;
    AudioChannel *channels;
    uint16_t channel_count;
    volatile uint8_t pending_halves;
} AudioMixer;

/* Register a fixed channel array once, before starting I2S. All buffers,
 * descriptors and context objects must remain alive while in use. Source
 * buffers have a positive, even frame_count and are circular. Output has a
 * positive, even stereo frame_count (at most 32767 for the I2S HAL).
 * Buffer pointers, sizes, formats and callbacks remain fixed after init.
 * Init clears the output to silence and resets source positions. Source
 * buffers must be populated by the caller before playback.
 * channels may be NULL when channel_count is zero. No allocation is done.
 */
void AudioMixer_Init(AudioMixer *mixer, AudioData *output,
                     AudioChannel *channels, uint16_t channel_count);

/* Call every main-loop pass; refill a released output half within 10 ms for
 * the 960-frame I2S buffer. Playback and gain changes are main-loop only.
 * STOPPED contributes silence and resets position; subsequent PLAY starts
 * at frame zero. PLAY_TO_END consumes to the next source half/full boundary,
 * invokes the source callback, then becomes STOPPED.
 * Service must observe STOPPED before a channel is restarted.
 *
 * Mono feeds both output sides, with independent gains; stereo scales each
 * corresponding side. Volumes are clamped to 0.0f through 1.0f and scaled
 * by 0.5f (approximately -6 dB). Summed float output saturates to int16_t,
 * with conversion truncating toward zero. Muted channels still advance.
 * Gains are sampled once at the start of each output-half refill.
 * Already queued output delays audible playback/gain changes.
 *
 * Source half/full callbacks run in this service in main-loop context.
 * Keep them non-blocking so the output refill meets its deadline.
 * Refills may be asynchronous: the mixer assumes data is ready whenever it
 * reads it, without waiting or tracking readiness. The caller must complete
 * refills before reuse. Do not recursively call service from a callback.
 */
void AudioMixer_Service(AudioMixer *mixer);

/* I2S callback entry point: context is AudioMixer*. Called in interrupt
 * context when an output half has been consumed. Only records the released
 * half; mixing and source callbacks run in service. If service misses a
 * notification, only the most recently released half is retained, avoiding
 * refilling an older half that DMA has already resumed reading. Service
 * must finish before DMA reuses the released half.
 */
void AudioMixer_OutputConsumed(void *context, AudioHalf half);

#ifdef __cplusplus
}
#endif

#endif // AUDIO_MIXER_H
