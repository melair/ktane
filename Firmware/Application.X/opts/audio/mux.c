#include <xc.h>
#include <stdint.h>
#include "audio.h"
#include "mux.h"
#include "../../opts.h"
#include "../spi/sdcard.h"
#include "../../hal/spi.h"
#include "../../hal/pins.h"
#include "../../malloc.h"

/*
 * We're limiting the mux to be able to handle 4 concurrent playbacks, this
 * assumes that getting the data into the PIC is via DMA. Mixing the audio
 * is the most expensive operation the bomb performs.
 *
 * We can actually manage 5, sometimes 6 - but it would leave no compute
 * resource for operating the rest of the module.
 *
 * The initial channel could be DMA'd, however it would essentially lock the
 * MCU data bus while it did it - because we aren't interrupt based, we can't
 * reduce the priority of the DMA and let it copy while idle.
 */
#define MUX_CHANNELS 4

typedef struct {
    uint16_t id;
    uint32_t block;
    uint32_t remaining;
} mux_channel_t;

mux_channel_t mux_channels[MUX_CHANNELS];
bool mux_mix_next_block = false;
bool mux_need_block = false;
bool mux_muxing = false;
uint16_t mux_buffer_base = 0;
uint8_t mux_ch = 0;
uint8_t *mux_buffer;

sd_transaction_t mux_sd_trans;

opt_data_t *audio;
opt_data_t *sdcard;

void mux_audio_request(opt_audio_t *a);
spi_command_t *mux_sdcard_callback(spi_command_t *cmd);
bool mux_start_next_transfer(void);

void mux_initialise(void) {
    audio = opts_find_audio();
    sdcard = opts_find_sdcard();

    mux_buffer = kmalloc(AUDIO_FRAME_SIZE + 12);

    audio_register_callback(&audio->audio, mux_audio_request);

    mux_sd_trans.spi_cmd.callback = mux_sdcard_callback;
    mux_sd_trans.spi_cmd.callback_ptr = NULL;
}

void mux_play(uint16_t id, uint32_t block, uint32_t count) {
    for (uint8_t i = 0; i < MUX_CHANNELS; i++) {
        if (mux_channels[i].remaining == 0) {
            mux_channels[i].id = id;
            mux_channels[i].block = block;
            mux_channels[i].remaining = count;
            return;
        }
    }
}

void mux_stop(uint16_t id) {
    for (uint8_t i = 0; i < MUX_CHANNELS; i++) {
        if (mux_channels[i].id == id) {
            mux_channels[i].id = 0;
            mux_channels[i].block = 0;
            mux_channels[i].remaining = 0;
        }
    }
}

void mux_audio_request(opt_audio_t *a) {
    if (!sdcard_is_ready(sdcard) || mux_muxing) {
        return;
    }

    mux_muxing = true;
    mux_mix_next_block = false;
    mux_buffer_base = (a->buffer_next ? AUDIO_FRAME_SIZE : 0);
    mux_ch = 0;

    if (!mux_start_next_transfer()) {
        for (uint16_t i = 0; i < AUDIO_FRAME_SIZE; i++) {
            a->buffer[mux_buffer_base + i] = 0x7f;
        }

        mux_muxing = false;
    } else {
        spi_enqueue(&mux_sd_trans.spi_cmd);
    }
}

bool mux_start_next_transfer(void) {
    bool found = false;

    for (uint8_t i = mux_ch; i < MUX_CHANNELS; i++) {
        if (mux_channels[i].remaining != 0) {
            mux_ch = i;
            found = true;
            break;
        }
    }

    if (found) {
        sdcard_transaction_read_block(&mux_sd_trans, sdcard, &mux_buffer[0], mux_channels[mux_ch].block);
        sdcard_transaction_callback(&mux_sd_trans);

        mux_channels[mux_ch].block++;
        mux_channels[mux_ch].remaining--;

        return true;
    } else {
        return false;
    }
}

void mux_service(void) {
    if (mux_need_block) {
        mux_need_block = false;

        if (mux_start_next_transfer()) {
            spi_enqueue(&mux_sd_trans.spi_cmd);
        } else {
            mux_muxing = false;
        }
    }
}

spi_command_t *mux_sdcard_callback(spi_command_t *cmd) {
    switch (sdcard_transaction_callback(&mux_sd_trans)) {
        case SDCARD_CMD_ERROR:
            return NULL;
        case SDCARD_CMD_COMPLETE:
            if (mux_mix_next_block) {
                uint16_t w;

                for (uint16_t i = 0; i < AUDIO_FRAME_SIZE; i++) {
                    uint8_t a = audio->audio.buffer[mux_buffer_base + i];
                    uint8_t b = mux_buffer[i];

                    /* http://www.vttoth.com/CMS/index.php/technical-notes/68 */
                    if (a < 128 && b < 128) {
                        w = (a * b) >> 7;
                    } else {
                        w = ((a + b + a + b)) - ((a * b) >> 7) - 256;
                    }

                    audio->audio.buffer[mux_buffer_base + i] = (uint8_t) w;
                }
            } else {
                for (uint16_t i = 0; i < AUDIO_FRAME_SIZE; i++) {
                    audio->audio.buffer[mux_buffer_base + i] = mux_buffer[i];
                }
            }

            mux_mix_next_block = true;
            mux_need_block = true;
            mux_ch++;

            return NULL;
    }

    return cmd;
}