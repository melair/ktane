#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* A frame is one mono sample or one interleaved stereo left/right pair.
 * All audio is signed 16-bit PCM at 48 kHz.
 */
typedef enum {
    AUDIO_HALF_FIRST,
    AUDIO_HALF_SECOND,
} AudioHalf;

/* Reports the consumed half, now available for refilling. SECOND corresponds
 * to the full-buffer boundary. NULL callbacks are allowed.
 */
typedef void (*AudioBufferCallback)(void *context, AudioHalf half);

/* Caller-owned stereo output buffer, shared by the mixer and I2S. */
typedef struct {
    int16_t *buffer;
    uint16_t frame_count;
} AudioData;

typedef enum {
    AUDIO_MONO = 1,
    AUDIO_STEREO = 2,
} AudioFormat;

typedef enum {
    AUDIO_STOPPED,
    AUDIO_PLAY,
    /* Play to the next source half/full boundary, then stop. */
    AUDIO_PLAY_TO_END,
} AudioPlayback;

typedef struct {
    int16_t *buffer;
    uint16_t frame_count;
    AudioFormat format;

    AudioPlayback playback;
    float volume_left;
    float volume_right;

    /* Half/full callbacks run in AudioMixer_Service, in the main loop.
     * Keep them non-blocking so the output refill meets its deadline.
     */
    AudioBufferCallback callback;
    void *context;

    /* Implementation-owned; callers must not modify. */
    struct {
        uint16_t read_frame;
        AudioPlayback previous_playback;
        float gain_left;
        float gain_right;
    } internal;
} AudioChannel;

#ifdef __cplusplus
}
#endif

#endif // AUDIO_H
