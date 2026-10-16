#ifndef PLAYER_H
#define PLAYER_H

#include <stdbool.h>

#include "audio/audio.h"
#include "fat/fat.h"

#define PLAYER_CHANNEL_COUNT 8u
/* 8 KiB of mono int16 PCM per channel; each half holds eight 512-byte sectors. */
#define PLAYER_CHANNEL_FRAME_COUNT 4096u
#define PLAYER_CHANNEL_SAMPLE_COUNT PLAYER_CHANNEL_FRAME_COUNT

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PLAYER_IDLE,
    PLAYER_LOOKUP_PENDING,
    PLAYER_PRIMING,
    PLAYER_PLAYING,
    PLAYER_STOPPING,
} Player_State;

typedef struct {
    /* Word member supplies the four-byte alignment required by SD DMA. */
    union {
        int16_t buffer[PLAYER_CHANNEL_SAMPLE_COUNT];
        uint32_t buffer_words[PLAYER_CHANNEL_SAMPLE_COUNT / 2u];
    };
    AudioChannel *channel;
    FAT_Request lookup;
    BlockIO_Transaction transaction;
    Player_State state;
    uint32_t next_sector;
    uint32_t remaining_bytes;
    uint32_t read_bytes;
    uint32_t fade_started_at;
    float fade_volume_left;
    float fade_volume_right;
    uint16_t fade_ms;
    uint16_t request_id;
    uint8_t identifier;
    uint8_t pending_halves;
    uint8_t ready_halves;
    uint8_t read_half;
    uint8_t final_half; /* 2 means the final half has not been loaded yet. */
    bool read_pending;
    bool lookup_pending;
    bool fading;
} Player_ChannelData;

typedef struct {
    AudioChannel channels[PLAYER_CHANNEL_COUNT];
    Player_ChannelData channel_data[PLAYER_CHANNEL_COUNT];
} Player_Data;

/* Initialise stopped, silent mono channels before registering them with the mixer.
 * Do not reinitialise with outstanding I/O.
 */
void Player_Init(void);

/* Refill consumed source halves each main-loop pass, after mixer service. */
void Player_Service(void);

/* Main-loop only. False means no free channel, a duplicate sender/request ID,
 * or rejected FAT submission.
 * Lookup completion is deferred; playback starts after source buffers are ready.
 * name is an exact FAT_NAME_SIZE-byte key, NUL padded if shorter.
 * Left/right volumes are linear gains from 0.0f to 1.0f, clamped by the mixer.
 */
bool Player_Play(uint8_t identifier, uint16_t request_id, uint8_t mode_id,
                 const char name[FAT_NAME_SIZE], float volume_left, float volume_right);

/* fade_ms = 0 stops immediately; otherwise linearly fade the current gains to
 * zero. A repeated stop restarts the fade from the current gains. Samples still
 * waiting for lookup/priming stop immediately. Outstanding I/O drains before reuse.
 */
void Player_Stop(uint8_t identifier, uint16_t request_id, uint16_t fade_ms);

/* Stop every channel using Player_Stop semantics; 0 stops immediately. */
void Player_StopAll(uint16_t fade_ms);

/* Mixer source callback; context is Player_ChannelData*, in main-loop context. */
void Player_BufferConsumed(void *context, AudioHalf half);

#ifdef __cplusplus
}
#endif

#endif // PLAYER_H
