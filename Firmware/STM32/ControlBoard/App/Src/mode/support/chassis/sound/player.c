#include "mode/support/chassis/sound/player.h"

#include <string.h>

#include "mode.h"
#include "stm32h5xx_hal.h"
#include "sys/sdcard.h"

#define PLAYER_HALF_BYTES (PLAYER_CHANNEL_SAMPLE_COUNT * sizeof(int16_t) / 2u)
#define PLAYER_NO_FINAL_HALF 2u

_Static_assert(PLAYER_HALF_BYTES % FAT_SECTOR_SIZE == 0u, "Player halves must hold whole sectors");
_Static_assert(offsetof(Player_ChannelData, buffer) % 4u == 0u, "SD buffer must be aligned");
_Static_assert(_Alignof(Player_ChannelData) >= 4u, "SD buffer must be aligned");

static Player_Data *const player = &mode_data.mode.chassis.player;

static void stop_channel(Player_ChannelData *data) {
    data->channel->playback = AUDIO_STOPPED;
    data->pending_halves = 0u;
    data->state = PLAYER_STOPPING;
    data->fading = false;
}

static void service_fade(Player_ChannelData *data, uint32_t now) {
    if (!data->fading) {
        return;
    }
    const uint32_t elapsed = now - data->fade_started_at;
    if (elapsed >= data->fade_ms) {
        data->channel->volume_left = 0.0f;
        data->channel->volume_right = 0.0f;
        stop_channel(data);
        return;
    }
    const float gain = 1.0f - (float) elapsed / data->fade_ms;
    data->channel->volume_left = data->fade_volume_left * gain;
    data->channel->volume_right = data->fade_volume_right * gain;
}

static void lookup_complete(FAT_Request *request) {
    Player_ChannelData *data = request->callback_data;
    data->lookup_pending = false;
    if (data->state != PLAYER_LOOKUP_PENDING) {
        return;
    }

    const FAT_File *file = &request->result;
    if (request->status != FAT_STATUS_SUCCESS || file->size_bytes == 0u ||
        file->size_bytes > FAT_MAX_FILE_SIZE || file->size_bytes % sizeof(int16_t) != 0u ||
        file->starting_sector == FAT_LAST_SECTOR ||
        file->sector_count != (file->size_bytes + FAT_SECTOR_SIZE - 1u) / FAT_SECTOR_SIZE ||
        file->last_sector_bytes_count != (file->size_bytes - 1u) % FAT_SECTOR_SIZE + 1u) {
        stop_channel(data);
        return;
    }

    data->next_sector = file->starting_sector;
    data->remaining_bytes = file->size_bytes;
    data->pending_halves = (1u << AUDIO_HALF_FIRST) | (1u << AUDIO_HALF_SECOND);
    data->state = PLAYER_PRIMING;
}

static BlockIO_Transaction *read_complete(BlockIO_Transaction *transaction) {
    Player_ChannelData *data = transaction->callback_data;
    data->read_pending = false;
    if (data->state == PLAYER_STOPPING) {
        return NULL;
    }
    if (transaction->status != BLOCK_IO_STATUS_SUCCESS) {
        stop_channel(data);
        return NULL;
    }

    /* Remove padding bytes from the last sector as well as unused sectors. */
    memset((uint8_t *) transaction->data + data->read_bytes, 0,
           PLAYER_HALF_BYTES - data->read_bytes);
    data->next_sector += transaction->sector_count;
    data->remaining_bytes -= data->read_bytes;
    data->ready_halves |= (uint8_t) (1u << data->read_half);
    if (data->remaining_bytes == 0u) {
        data->final_half = data->read_half;
        data->pending_halves = 0u;
    }
    return NULL;
}

static void queue_read(Player_ChannelData *data, uint8_t half) {
    data->read_half = half;
    data->read_bytes = data->remaining_bytes < PLAYER_HALF_BYTES
        ? data->remaining_bytes : PLAYER_HALF_BYTES;
    data->transaction = (BlockIO_Transaction) {
        .operation = BLOCK_IO_OPERATION_READ,
        .sector = data->next_sector,
        .sector_count = (data->read_bytes + FAT_SECTOR_SIZE - 1u) / FAT_SECTOR_SIZE,
        .data = (uint8_t *) data->buffer + half * PLAYER_HALF_BYTES,
        .callback = read_complete,
        .callback_data = data,
    };
    if (!SDCard_Queue(&data->transaction)) {
        stop_channel(data);
        return;
    }
    data->read_pending = true;
    data->pending_halves &= (uint8_t) ~(1u << half);
}

void Player_Init(void) {
    for (uint16_t index = 0u; index < PLAYER_CHANNEL_COUNT; ++index) {
        Player_ChannelData *data = &player->channel_data[index];
        memset(data, 0, sizeof(*data));
        data->channel = &player->channels[index];
        data->final_half = PLAYER_NO_FINAL_HALF;
        *data->channel = (AudioChannel) {
            .buffer = data->buffer,
            .frame_count = PLAYER_CHANNEL_FRAME_COUNT,
            .format = AUDIO_MONO,
            .playback = AUDIO_STOPPED,
            .volume_left = 1.0f,
            .volume_right = 1.0f,
            .callback = Player_BufferConsumed,
            .context = data,
        };
    }
}

bool Player_Play(uint8_t identifier, uint16_t request_id, uint8_t mode_id,
                 const char name[FAT_NAME_SIZE], float volume_left, float volume_right) {
    Player_ChannelData *available = NULL;
    for (uint16_t index = 0u; index < PLAYER_CHANNEL_COUNT; ++index) {
        Player_ChannelData *data = &player->channel_data[index];
        if (data->state != PLAYER_IDLE && data->identifier == identifier &&
            data->request_id == request_id) {
            return false;
        }
        if (data->state == PLAYER_IDLE && available == NULL) {
            available = data;
        }
    }
    if (available == NULL) {
        return false;
    }

    memset(&available->lookup, 0, sizeof(available->lookup));
    available->lookup.key.mode_id = mode_id;
    available->lookup.key.type_id = FAT_TYPE_PCM;
    memcpy(available->lookup.key.name, name, sizeof(available->lookup.key.name));

    available->identifier = identifier;
    available->request_id = request_id;
    available->pending_halves = 0u;
    available->ready_halves = 0u;
    available->final_half = PLAYER_NO_FINAL_HALF;
    available->fading = false;
    available->channel->volume_left = volume_left;
    available->channel->volume_right = volume_right;
    available->lookup.callback = lookup_complete;
    available->lookup.callback_data = available;
    available->state = PLAYER_LOOKUP_PENDING;
    const FAT_Status status = FAT_Find(&available->lookup);
    if (status != FAT_STATUS_PENDING) {
        stop_channel(available);
        return false;
    }
    available->lookup_pending = true;
    return true;
}

void Player_Stop(uint8_t identifier, uint16_t request_id, uint16_t fade_ms) {
    const uint32_t now = HAL_GetTick();
    for (uint16_t index = 0u; index < PLAYER_CHANNEL_COUNT; ++index) {
        Player_ChannelData *data = &player->channel_data[index];
        if (data->state != PLAYER_IDLE && data->identifier == identifier &&
            data->request_id == request_id) {
            service_fade(data, now);
            if (fade_ms == 0u || data->state != PLAYER_PLAYING) {
                stop_channel(data);
            } else {
                data->fade_started_at = now;
                data->fade_ms = fade_ms;
                data->fade_volume_left = data->channel->volume_left;
                data->fade_volume_right = data->channel->volume_right;
                data->fading = true;
            }
        }
    }
}

void Player_StopAll(uint16_t fade_ms) {
    for (uint16_t index = 0u; index < PLAYER_CHANNEL_COUNT; ++index) {
        Player_ChannelData *data = &player->channel_data[index];
        if (data->state != PLAYER_IDLE) {
            Player_Stop(data->identifier, data->request_id, fade_ms);
        }
    }
}

void Player_Service(void) {
    const uint32_t now = HAL_GetTick();
    for (uint16_t index = 0u; index < PLAYER_CHANNEL_COUNT; ++index) {
        Player_ChannelData *data = &player->channel_data[index];
        service_fade(data, now);
        if (data->state == PLAYER_STOPPING) {
            /* FAT/SD owns its request and buffers until callback delivery. */
            if (!data->read_pending && !data->lookup_pending) {
                data->state = PLAYER_IDLE;
            }
            continue;
        }
        if (data->state != PLAYER_PRIMING && data->state != PLAYER_PLAYING) {
            continue;
        }
        if (data->read_pending) {
            continue;
        }

        if (data->state == PLAYER_PRIMING &&
            (data->ready_halves == 3u || data->final_half == AUDIO_HALF_FIRST)) {
            data->state = PLAYER_PLAYING;
            data->channel->playback = data->final_half == AUDIO_HALF_FIRST
                ? AUDIO_PLAY_TO_END : AUDIO_PLAY;
        }
        if (data->remaining_bytes != 0u && data->pending_halves != 0u) {
            const uint8_t half = (data->pending_halves & (1u << AUDIO_HALF_FIRST))
                ? AUDIO_HALF_FIRST : AUDIO_HALF_SECOND;
            queue_read(data, half);
        }
    }
}

void Player_BufferConsumed(void *context, AudioHalf half) {
    Player_ChannelData *data = context;
    data->ready_halves &= (uint8_t) ~(1u << half);
    if (half == data->final_half) {
        stop_channel(data);
        return;
    }

    const uint8_t next_half = half == AUDIO_HALF_FIRST ? AUDIO_HALF_SECOND : AUDIO_HALF_FIRST;
    /* Do not let the mixer read stale data or a buffer still owned by SD DMA. */
    if ((data->ready_halves & (1u << next_half)) == 0u) {
        stop_channel(data);
        return;
    }
    if (next_half == data->final_half) {
        data->channel->playback = AUDIO_PLAY_TO_END;
    }
    if (data->remaining_bytes != 0u) {
        data->pending_halves |= (uint8_t) (1u << half);
    }
}
