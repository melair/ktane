#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include "cardscan.h"
#include "pn532.h"
#include "pn532_cmd.h"
#include "pn532_packet.h"
#include "../../hal/pins.h"
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

#define CARDSCAN_SAME_CARD_COOLDOWN 2500

void pn532_service(void) {
    uint8_t size;
    uint8_t writebuf[4] = {0xc0, 0xff, 0xee, 0x00};

    switch (mode_data.cardscan.pn532.state) {
        case PN532_STATE_POWER_ON_WAIT:
            if (mode_data.cardscan.pn532.wait_time + PN532_POWER_ON_WAIT < tick_value) {
                mode_data.cardscan.pn532.state = PN532_STATE_CONFIG;
            }

            break;

        case PN532_STATE_CONFIG:
            mode_data.cardscan.pn532.spi.cmd.pre_delay = 500;
            mode_data.cardscan.pn532.state = PN532_STATE_CONFIG_WAIT;
            size = pn532_samconfigure(&mode_data.cardscan.pn532.spi.buffer[0]);
            pn532_cmd_send(&mode_data.cardscan.pn532.spi.buffer[0], size, 9, &pn532_service_samconfigure_callback);
            break;

        case PN532_STATE_CONFIG_WAIT:
            break;

        case PN532_STATE_DETECT_START:
            mode_data.cardscan.pn532.spi.cmd.pre_delay = 0;
            mode_data.cardscan.pn532.state = PN532_STATE_DETECT_START_ACK;
            size = pn532_inlistpassivetarget(&mode_data.cardscan.pn532.spi.buffer[0]);
            pn532_cmd_send(&mode_data.cardscan.pn532.spi.buffer[0], size, 0, &pn532_service_inpassivetarget_callback);
            break;

        case PN532_STATE_DETECT_START_ACK:
            break;

        case PN532_STATE_DETECT_WAIT:
            if (!kpin_read(KPIN_C4)) {
                mode_data.cardscan.pn532.state = PN532_STATE_CARD_FOUND_INFLIGHT;
                pn532_cmd_send(&mode_data.cardscan.pn532.spi.buffer[0], 0, 22, &pn532_service_detect_callback);
            }
            break;

        case PN532_STATE_CARD_FOUND_INFLIGHT:
            break;

        case PN532_STATE_BLOCK_READ:
            mode_data.cardscan.pn532.state = PN532_STATE_BLOCK_READ_INFLIGHT;
            size = pn532_mifareultralight_read(&mode_data.cardscan.pn532.spi.buffer[0], 4);
            pn532_cmd_send(&mode_data.cardscan.pn532.spi.buffer[0], size, 26, &pn532_service_mfu_read_callback);
            break;

        case PN532_STATE_BLOCK_READ_INFLIGHT:
            break;

        case PN532_STATE_BLOCK_WRITE:
            writebuf[3] = mode_data.cardscan.cards.programming_id;

            mode_data.cardscan.pn532.state = PN532_STATE_BLOCK_WRITE_INFLIGHT;
            size = pn532_mifareultralight_write(&mode_data.cardscan.pn532.spi.buffer[0], 4, &writebuf[0]);
            pn532_cmd_send(&mode_data.cardscan.pn532.spi.buffer[0], size, 10, &pn532_service_mfu_write_callback);
            break;

        case PN532_STATE_BLOCK_WRITE_INFLIGHT:
            break;

        case PN532_STATE_COOLDOWN:
            mode_data.cardscan.pn532.wait_time = tick_value;
            mode_data.cardscan.pn532.state = PN532_STATE_COOLDOWN_WAIT;
            break;

        case PN532_STATE_COOLDOWN_WAIT:
            if (mode_data.cardscan.pn532.wait_time + PN532_COOLDOWN < tick_value) {
                mode_data.cardscan.pn532.state = PN532_STATE_DETECT_START;
            }
            break;
    }
}

void pn532_service_inpassivetarget_callback(bool ok) {
    if (ok) {
        mode_data.cardscan.pn532.state = PN532_STATE_DETECT_WAIT;
    }
}

void pn532_service_samconfigure_callback(bool ok) {
    if (ok) {
        if (mode_data.cardscan.pn532.spi.buffer[6] == 0x15) {
            mode_data.cardscan.pn532.state = PN532_STATE_DETECT_START;
        }
    }
}

void pn532_service_detect_callback(bool ok) {
    if (ok) {
        if (mode_data.cardscan.pn532.spi.buffer[5] == 0xd5 && mode_data.cardscan.pn532.spi.buffer[6] == 0x4b && mode_data.cardscan.pn532.spi.buffer[7] == 0x01) {
            uint16_t atqa = (uint16_t) ((mode_data.cardscan.pn532.spi.buffer[9] << 8) | mode_data.cardscan.pn532.spi.buffer[10]);
            uint8_t sak = mode_data.cardscan.pn532.spi.buffer[11];
            uint8_t uid_len = mode_data.cardscan.pn532.spi.buffer[12];

            /* Verify if a Mifare Ultralight. */
            if (atqa == 0x0044 && sak == 0x00 && uid_len == CARDSCAN_MFU_UID_LEN) {
                bool different_card = false;

                // Check if card is different, if it is set flag so. Also update
                // the last scanned UID in same sweep. If the UID is the same
                // then this doesn't matter, and if it is we want it updated.
                for (uint8_t i = 0; i < CARDSCAN_MFU_UID_LEN; i++) {
                    if (mode_data.cardscan.pn532.spi.buffer[13 + i] != mode_data.cardscan.rfid.scan_uid[i]) {
                        mode_data.cardscan.rfid.scan_uid[i] = mode_data.cardscan.pn532.spi.buffer[13 + i];
                        different_card = true;
                    }
                }

                if (different_card || (mode_data.cardscan.rfid.scan_tick + CARDSCAN_SAME_CARD_COOLDOWN < tick_value)) {
                    if (mode_data.cardscan.cards.programming) {
                        mode_data.cardscan.pn532.state = PN532_STATE_BLOCK_WRITE;
                    } else {
                        mode_data.cardscan.pn532.state = PN532_STATE_BLOCK_READ;
                    }

                    mode_data.cardscan.rfid.scan_tick = tick_value;
                } else {
                    mode_data.cardscan.pn532.state = PN532_STATE_COOLDOWN;
                }
            } else {
                mode_data.cardscan.pn532.state = PN532_STATE_COOLDOWN;
            }
        } else {
            mode_data.cardscan.pn532.state = PN532_STATE_COOLDOWN;
        }
    }
}

void pn532_service_mfu_read_callback(bool ok) {
    if (ok) {
        if (mode_data.cardscan.pn532.spi.buffer[5] == 0xd5 && mode_data.cardscan.pn532.spi.buffer[6] == 0x41) {
            if (mode_data.cardscan.pn532.spi.buffer[7] == 0x00 &&
                    mode_data.cardscan.pn532.spi.buffer[8] == 0xc0 &&
                    mode_data.cardscan.pn532.spi.buffer[9] == 0xff &&
                    mode_data.cardscan.pn532.spi.buffer[10] == 0xee) {

                // Read Success
                mode_data.cardscan.cards.scanned_id = mode_data.cardscan.pn532.spi.buffer[11];
                mode_data.cardscan.cards.scanned_updated = true;
            }
        }
    }

    mode_data.cardscan.pn532.state = PN532_STATE_COOLDOWN;
}

void pn532_service_mfu_write_callback(bool ok) {
    if (ok) {
        if (mode_data.cardscan.pn532.spi.buffer[5] == 0xd5 && mode_data.cardscan.pn532.spi.buffer[6] == 0x41) {
            if (mode_data.cardscan.pn532.spi.buffer[7] == 0x00 && (mode_data.cardscan.pn532.spi.buffer[8] & 0x0f) == 0x0a) {
                // Write Success!
                mode_data.cardscan.cards.programming_id++;
                mode_data.cardscan.cards.programming_update = true;

                if (mode_data.cardscan.cards.programming_id >= CARDSCAN_CARD_COUNT) {
                    mode_data.cardscan.cards.programming_id = 0;
                }
            }
        }
    }

    mode_data.cardscan.pn532.state = PN532_STATE_COOLDOWN;
}
