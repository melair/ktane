#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include "pn532.h"
#include "pn532_spi.h"
#include "pn532_packet.h"
#include "../../peripherals/ports.h"
#include "../../mode.h"
#include "../../tick.h"
#include "../../buzzer.h"

#define PN532_STATE_POWER_ON_WAIT        0
#define PN532_STATE_CONFIG               1
#define PN532_STATE_CONFIG_WAIT          2
#define PN532_STATE_DETECT_START         3
#define PN532_STATE_DETECT_START_ACK     4
#define PN532_STATE_DETECT_WAIT          5
#define PN532_STATE_CARD_FOUND_INFLIGHT  6
#define PN532_STATE_BLOCK_READ           7
#define PN532_STATE_BLOCK_READ_INFLIGHT  8
#define PN532_STATE_BLOCK_WRITE          9
#define PN532_STATE_BLOCK_WRITE_INFLIGHT 10
#define PN532_STATE_COOLDOWN             11
#define PN532_STATE_COOLDOWN_WAIT        12

#define PN532_POWER_ON_WAIT 500
#define PN532_COOLDOWN      250

#define CARDSCAN_SIZE 16
bool cardscan_prog = false;
uint8_t cardscan_prog_id = 0;

#define CARDSCAN_SAME_CARD_COOLDOWN 2500

uint32_t cardscan_last_scan;
uint8_t cardscan_last_uid[7];
uint8_t cardscan_game_id;
bool cardscan_game_id_new = false;

void pn532_service(void) {  
    uint8_t size;
    uint8_t writebuf[4] = { 0xc0, 0xff, 0xee, 0x00 };

    switch(mode_data.cardscan.pn532_state) {
        case PN532_STATE_POWER_ON_WAIT:
            if (mode_data.cardscan.pn532_wait_time + PN532_POWER_ON_WAIT < tick_value) {
                mode_data.cardscan.pn532_state = PN532_STATE_CONFIG;
            }

            break;
            
        case PN532_STATE_CONFIG:
            kpin_write(KPIN_C3, false);
            mode_data.cardscan.pn532_state = PN532_STATE_CONFIG_WAIT;
            size = pn532_samconfigure(&mode_data.cardscan.spi_buffer);
            pn532_spi_send(&mode_data.cardscan.spi_buffer[0], size, 9, &pn532_service_samconfigure_callback);         
            break;
            
        case PN532_STATE_CONFIG_WAIT:
            // TODO: Jump back if config times out.
            break;
            
        case PN532_STATE_DETECT_START:
            kpin_write(KPIN_C3, false);
            mode_data.cardscan.pn532_state = PN532_STATE_DETECT_START_ACK;
            size = pn532_inlistpassivetarget(&mode_data.cardscan.spi_buffer);
            pn532_spi_send(&mode_data.cardscan.spi_buffer[0], size, 0, &pn532_service_inpassivetarget_callback);  
            break;
            
        case PN532_STATE_DETECT_START_ACK:
            break;
           
        case PN532_STATE_DETECT_WAIT:
            if (!kpin_read(KPIN_C4)) {
                mode_data.cardscan.pn532_state = PN532_STATE_CARD_FOUND_INFLIGHT;
                pn532_spi_send(&mode_data.cardscan.spi_buffer[0], 0, 22, &pn532_service_detect_callback);  
            }
            break;

        case PN532_STATE_CARD_FOUND_INFLIGHT:
            break;
            
        case PN532_STATE_BLOCK_READ:
            kpin_write(KPIN_C3, false);
            mode_data.cardscan.pn532_state = PN532_STATE_BLOCK_READ_INFLIGHT;
            size = pn532_mifareultralight_read(&mode_data.cardscan.spi_buffer, 4);
            pn532_spi_send(&mode_data.cardscan.spi_buffer[0], size, 26, &pn532_service_mfu_read_callback);              
            break;
            
        case PN532_STATE_BLOCK_READ_INFLIGHT:
            break;
            
        case PN532_STATE_BLOCK_WRITE:
            writebuf[3] = cardscan_prog_id;
            
            kpin_write(KPIN_C3, false);
            mode_data.cardscan.pn532_state = PN532_STATE_BLOCK_WRITE_INFLIGHT;
            size = pn532_mifareultralight_write(&mode_data.cardscan.spi_buffer, 4, &writebuf);
            pn532_spi_send(&mode_data.cardscan.spi_buffer[0], size, 10, &pn532_service_mfu_write_callback); 
            break;
            
        case PN532_STATE_BLOCK_WRITE_INFLIGHT:
            break;
            
        case PN532_STATE_COOLDOWN:
            mode_data.cardscan.pn532_wait_time = tick_value;
            mode_data.cardscan.pn532_state = PN532_STATE_COOLDOWN_WAIT;
            break;
            
        case PN532_STATE_COOLDOWN_WAIT:
            if (mode_data.cardscan.pn532_wait_time + PN532_COOLDOWN < tick_value) {
                mode_data.cardscan.pn532_state = PN532_STATE_DETECT_START;
            }
            break;
    }        
}

void pn532_service_inpassivetarget_callback(bool ok) {
    if (ok) {
        mode_data.cardscan.pn532_state = PN532_STATE_DETECT_WAIT;   
    } else {
        
    }
}

void pn532_service_samconfigure_callback(bool ok) {
    if (ok) {
        if (mode_data.cardscan.spi_buffer[6] == 0x15) {
            mode_data.cardscan.pn532_state = PN532_STATE_DETECT_START;
        } else {
            // Error condition.
        }       
    } else {
        
    }
}

void pn532_service_detect_callback(bool ok) {
    if (ok) {                      
        if (mode_data.cardscan.spi_buffer[5] == 0xd5 && mode_data.cardscan.spi_buffer[6] == 0x4b && mode_data.cardscan.spi_buffer[7] == 0x01) {
            uint16_t atqa = (mode_data.cardscan.spi_buffer[9] << 8) | mode_data.cardscan.spi_buffer[10];
            uint8_t sak = mode_data.cardscan.spi_buffer[11];
            uint8_t uid_len = mode_data.cardscan.spi_buffer[12];

            /* Verify if a Mifare Ultralight. */
            if (atqa == 0x0044 && sak == 0x00 && uid_len == 0x07) {                   
                bool different_card = false;
                
                // Check if card is different, if it is set flag so. Also update
                // the last scanned UID in same sweep. If the UID is the same
                // then this doesn't matter, and if it is we want it updated.
                for (uint8_t i = 0; i < uid_len; i++) {
                    if (mode_data.cardscan.spi_buffer[13+i] != cardscan_last_uid[i]) {
                        different_card = true;
                    }
                    
                    cardscan_last_uid[i] = mode_data.cardscan.spi_buffer[13+i];
                }
                
                if (different_card || (cardscan_last_scan + CARDSCAN_SAME_CARD_COOLDOWN < tick_value)) {                    
                    if (cardscan_prog) {
                        mode_data.cardscan.pn532_state = PN532_STATE_BLOCK_WRITE;
                    } else {
                        mode_data.cardscan.pn532_state = PN532_STATE_BLOCK_READ;
                    }
                    
                    cardscan_last_scan = tick_value;
                } else {
                    mode_data.cardscan.pn532_state = PN532_STATE_COOLDOWN;
                }               
            } else {
                mode_data.cardscan.pn532_state = PN532_STATE_COOLDOWN;
            }
        } else {
            mode_data.cardscan.pn532_state = PN532_STATE_COOLDOWN;
        }
    }
}

void pn532_service_mfu_read_callback(bool ok) {
    if (ok) {                      
        if (mode_data.cardscan.spi_buffer[5] == 0xd5 && mode_data.cardscan.spi_buffer[6] == 0x41) {
            if (mode_data.cardscan.spi_buffer[7] == 0x00 &&
                mode_data.cardscan.spi_buffer[8] == 0xc0 &&
                mode_data.cardscan.spi_buffer[9] == 0xff &&
                mode_data.cardscan.spi_buffer[10] == 0xee) {

                // Read Success
                cardscan_game_id = mode_data.cardscan.spi_buffer[10];
                cardscan_game_id_new = true;
                
                buzzer_on_timed(BUZZER_DEFAULT_VOLUME, BUZZER_FREQ_A6_SHARP, 100);
            }
        }
    }
    
    mode_data.cardscan.pn532_state = PN532_STATE_COOLDOWN;
}

void pn532_service_mfu_write_callback(bool ok) {
    if (ok) {                      
        if (mode_data.cardscan.spi_buffer[5] == 0xd5 && mode_data.cardscan.spi_buffer[6] == 0x41) {
            if (mode_data.cardscan.spi_buffer[7] == 0x00 && (mode_data.cardscan.spi_buffer[8] & 0x0f) == 0x0a) {
                // Write Success!
                cardscan_prog_id++;
                
                if (cardscan_prog_id >= CARDSCAN_SIZE) {
                    cardscan_prog = 0;
                }
                
                buzzer_on_timed(BUZZER_DEFAULT_VOLUME, BUZZER_FREQ_A6_SHARP, 100);
            } 
        } 
    }
    
    mode_data.cardscan.pn532_state = PN532_STATE_COOLDOWN;
}
