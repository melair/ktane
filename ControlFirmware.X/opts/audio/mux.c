#include <xc.h>
#include <stdint.h>
#include "audio.h"
#include "../../opts.h"
#include "../spi/sdcard.h"
#include "../../hal/spi.h"

uint8_t mux_buffer[AUDIO_FRAME_SIZE + 12];

#define MUX_CHANNELS 4

typedef struct {
    uint32_t block;
    uint32_t remaining;
} mux_channel_t;

mux_channel_t mux_channels[MUX_CHANNELS];
bool mux_mix = false;
uint16_t mux_buffer_base = 0;
uint8_t mux_ch = 0;

sd_transaction_t mux_sd_trans;

opt_data_t *audio;
opt_data_t *sdcard;

void mux_audio_request(opt_audio_t *a);
spi_command_t *mux_sdcard_callback(spi_command_t *cmd);
bool mux_start_next_transfer(void);

void mux_initialise(void) {
    audio = opts_find_audio();
    sdcard = opts_find_sdcard();
    
    audio_register_callback(&audio->audio, mux_audio_request);
    
    mux_sd_trans.spi_cmd.callback = mux_sdcard_callback;
    mux_sd_trans.spi_cmd.callback_ptr = NULL;
    
    mux_channels[0].block = 0;
    mux_channels[0].remaining = 32 * 60;
    
    mux_channels[1].block = mux_channels[0].remaining;
    mux_channels[1].remaining = 32 * 60;
}

void mux_play(uint32_t block, uint32_t count) {
    for (uint8_t i = 0; i < MUX_CHANNELS; i++) {
        if (mux_channels[i].remaining == 0) {
            mux_channels[i].block = block;
            mux_channels[i].remaining = count;
            return;
        }
    }
}

void mux_audio_request(opt_audio_t *a) {
    if (!sdcard_is_ready(sdcard)) {
        return;
    }
    
    mux_mix = false;
    mux_buffer_base = (a->buffer_next ? AUDIO_FRAME_SIZE : 0);
    mux_ch = 0;
              
    if (!mux_start_next_transfer()) {
        for (uint16_t i = 0; i < AUDIO_FRAME_SIZE; i++) {
            a->buffer[mux_buffer_base+i] = 0x7f;
        }
    } else {
        spi_enqueue(&mux_sd_trans.spi_cmd);
    }
}

bool mux_start_next_transfer(void) {   
    bool found = false;
    
    for(uint8_t i = mux_ch; i < MUX_CHANNELS; i++) {
        if (mux_channels[i].remaining != 0) {
            mux_ch = i;
            found = true;
            break;
        }
    }
    
    if (found) {
        sdcard_transaction_read_block(&mux_sd_trans, sdcard, &mux_buffer, mux_channels[mux_ch].block);   
        sdcard_transaction_callback(&mux_sd_trans);

        mux_channels[mux_ch].block++;
        mux_channels[mux_ch].remaining--;    
        
        return true;
    } else {
        return false;
    }    
}

spi_command_t *mux_sdcard_callback(spi_command_t *cmd) {
    switch(sdcard_transaction_callback(&mux_sd_trans)) {
        case SDCARD_CMD_ERROR:
            return NULL;
        case SDCARD_CMD_COMPLETE:     
            if (mux_mix) {
                uint16_t w;
                
                for (uint16_t i = 0; i < AUDIO_FRAME_SIZE; i++) {
                    uint8_t a = audio->audio.buffer[mux_buffer_base+i];
                    uint8_t b = mux_buffer[i];
                    
                    /* http://www.vttoth.com/CMS/index.php/technical-notes/68 */
                    if (a < 128 && b < 128) {
                        w = (a * b) / 128;
                    } else {
                        w = (2 * (a + b)) - ((a * b) / 128) - 256;
                    }
                    
                    audio->audio.buffer[mux_buffer_base+i] = w;                   
                }
            } else {            
                for (uint16_t i = 0; i < AUDIO_FRAME_SIZE; i++) {
                    audio->audio.buffer[mux_buffer_base+i] = mux_buffer[i];
                }
            }
            
            mux_mix = true;
            mux_ch++;
            
            // Adding the chained SPI command causes this to fail. Second transaction never
            // seems to actually get invoked correctly.
            
//            if (mux_start_next_transfer()) {
//                return cmd;
//            } 
            
            return NULL;
    }
    
    return cmd;
}