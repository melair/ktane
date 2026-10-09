#include "audio_mixer.h"

#include <stdbool.h>
#include <string.h>

#include "stm32h5xx.h"

static float channel_gain(float volume) {
    /* Also treats NaN as muted. */
    if (!(volume > 0.0f)) {
        return 0.0f;
    }
    if (volume >= 1.0f) {
        return 0.5f;
    }
    return volume * 0.5f;
}

static int16_t output_sample(float sample) {
    if (sample >= 32767.0f) {
        return INT16_MAX;
    }
    if (sample <= -32768.0f) {
        return INT16_MIN;
    }
    return (int16_t) sample;
}

static void stop_channel(AudioChannel *channel) {
    channel->playback = AUDIO_STOPPED;
    channel->internal.read_frame = 0;
    channel->internal.previous_playback = AUDIO_STOPPED;
}

static void advance_channel(AudioChannel *channel) {
    const uint16_t next_frame = channel->internal.read_frame + 1u;
    const bool full = next_frame == channel->frame_count;
    channel->internal.read_frame = full ? 0u : next_frame;

    if (full || next_frame == channel->frame_count / 2u) {
        const bool stop = channel->playback == AUDIO_PLAY_TO_END;
        if (channel->callback != NULL) {
            channel->callback(channel->context,
                              full ? AUDIO_HALF_SECOND : AUDIO_HALF_FIRST);
        }
        if (stop || channel->playback == AUDIO_STOPPED) {
            stop_channel(channel);
        }
    }
}

void AudioMixer_Init(AudioMixer *mixer, AudioData *output,
                     AudioChannel *channels, uint16_t channel_count) {
    mixer->output = output;
    mixer->channels = channels;
    mixer->channel_count = channel_count;
    mixer->pending_halves = 0;

    memset(output->buffer, 0, (size_t) output->frame_count * 2u * sizeof(int16_t));
    for (uint16_t i = 0; i < channel_count; ++i) {
        channels[i].internal.read_frame = 0;
        channels[i].internal.previous_playback = AUDIO_STOPPED;
        channels[i].internal.gain_left = 0.0f;
        channels[i].internal.gain_right = 0.0f;
    }
}

void AudioMixer_Service(AudioMixer *mixer) {
    /* Observe stops even when DMA has not released an output half yet. */
    for (uint16_t i = 0; i < mixer->channel_count; ++i) {
        AudioChannel *channel = &mixer->channels[i];
        if (channel->playback == AUDIO_STOPPED ||
            channel->internal.previous_playback == AUDIO_STOPPED) {
            channel->internal.read_frame = 0;
        }
        channel->internal.previous_playback = channel->playback;
    }

    /* Claim the notification without losing an interrupt between read/clear.
     * Preserve the caller's interrupt mask rather than always enabling IRQs.
     */
    const uint32_t interrupt_mask = __get_PRIMASK();
    __disable_irq();
    const uint8_t pending = mixer->pending_halves;
    mixer->pending_halves = 0;
    __set_PRIMASK(interrupt_mask);
    if (pending == 0u) {
        return;
    }

    bool playing = false;
    for (uint16_t i = 0; i < mixer->channel_count; ++i) {
        AudioChannel *channel = &mixer->channels[i];
        channel->internal.gain_left = channel_gain(channel->volume_left);
        channel->internal.gain_right = channel_gain(channel->volume_right);
        playing |= channel->playback == AUDIO_PLAY || channel->playback == AUDIO_PLAY_TO_END;
    }

    const uint16_t frame_count = mixer->output->frame_count / 2u;
    int16_t *output = mixer->output->buffer;
    if (pending == (1u << AUDIO_HALF_SECOND)) {
        output += (uint32_t) frame_count * 2u;
    }

    // TODO - Why?
    if (!playing) {
        memset(output, 0, (size_t) frame_count * 2u * sizeof(int16_t));
        return;
    }

    for (uint16_t frame = 0; frame < frame_count; ++frame) {
        float left = 0.0f;
        float right = 0.0f;
        for (uint16_t i = 0; i < mixer->channel_count; ++i) {
            AudioChannel *channel = &mixer->channels[i];
            if (channel->playback != AUDIO_PLAY &&
                channel->playback != AUDIO_PLAY_TO_END) {
                continue;
            }
            /* A source callback may have started another channel this block. */
            channel->internal.previous_playback = channel->playback;

            const uint32_t source_frame = channel->internal.read_frame;
            if (channel->format == AUDIO_MONO) {
                const float sample = channel->buffer[source_frame];
                left += sample * channel->internal.gain_left;
                right += sample * channel->internal.gain_right;
            } else {
                left += channel->buffer[source_frame * 2u] * channel->internal.gain_left;
                right += channel->buffer[source_frame * 2u + 1u] * channel->internal.gain_right;
            }
            advance_channel(channel);
        }
        output[(uint32_t) frame * 2u] = output_sample(left);
        output[(uint32_t) frame * 2u + 1u] = output_sample(right);
    }
}

void AudioMixer_OutputConsumed(void *context, AudioHalf half) {
    AudioMixer *mixer = context;
    mixer->pending_halves = (uint8_t) (1u << half);
}
