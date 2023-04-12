#include <xc.h>
#include <stdint.h>
#include "../common/packet.h"
#include "../common/protocol.h"
#include "../common/fw.h"

/* Firmware state. */
#define FIRMWARE_PROCESS_IDLE       0
#define FIRMWARE_PROCESS_HEADER     1
#define FIRMWARE_PROCESS_PAGES      2
#define FIRMWARE_PROCESS_FAILED     3

#define FIRMWARE_ATTEMPTS 3

uint8_t fw_updater_state = FIRMWARE_PROCESS_IDLE;
uint8_t fw_updater_attempt = 0;

#define NO_TARGET_SEGMENT 0xff

uint8_t fw_updater_target_segment = NO_TARGET_SEGMENT;
uint16_t fw_updater_target_version;
uint32_t fw_updater_target_crc;
uint16_t fw_updater_target_pages_total;
uint16_t fw_updater_target_pages_current;
uint8_t fw_updater_firmware_source;

/* Location function prototypes. */
void fw_updater_request_page(void);

void fw_updater_start(uint8_t segment, uint16_t version) {
    if (fw_updater_state == FIRMWARE_PROCESS_IDLE) {
        fw_updater_target_segment = segment;
        fw_updater_target_version = version;

        packet_outgoing.firmware.request.segment = segment;
        packet_outgoing.firmware.request.version = version;
        packet_send(PREFIX_FIRMWARE, OPCODE_FIRMWARE_REQUEST, SIZE_FIRMWARE_REQUEST, &packet_outgoing);

        fw_updater_state = FIRMWARE_PROCESS_HEADER;
    }
}

void fw_updater_receive_header(uint8_t id, packet_t *p) {
    if (fw_updater_state == FIRMWARE_PROCESS_HEADER) {
        if (p->firmware.header.segment == fw_updater_target_segment && p->firmware.header.version == fw_updater_target_version) {
            fw_updater_firmware_source = id;
            fw_updater_target_pages_current = 0;
            fw_updater_target_pages_total = p->firmware.header.pages;
            fw_updater_target_crc = p->firmware.header.crc;

            fw_updater_request_page();

            fw_updater_state = FIRMWARE_PROCESS_PAGES;
        }
    }
}

void fw_updater_request_page(void) {
    packet_outgoing.firmware.page_request.page = fw_updater_target_pages_current;
    packet_outgoing.firmware.page_request.segment = fw_updater_target_segment;
    packet_outgoing.firmware.page_request.source_id = fw_updater_firmware_source;
    packet_send(PREFIX_FIRMWARE, OPCODE_FIRMWARE_PAGE_REQUEST, SIZE_FIRMWARE_PAGE_REQUEST, &packet_outgoing);
}

void fw_updater_receive_page(uint8_t id, packet_t *p) {
    if (fw_updater_state == FIRMWARE_PROCESS_PAGES) {
        if (id == fw_updater_firmware_source && p->firmware.page_response.segment == fw_updater_target_segment && p->firmware.page_response.page == fw_updater_target_pages_current) {
            fw_page_write(fw_updater_target_segment, fw_updater_target_pages_current, &p->firmware.page_response.data[0]);

            fw_updater_target_pages_current++;

            if (fw_updater_target_pages_current < fw_updater_target_pages_total) {
                fw_updater_request_page();
            } else {
                uint32_t new_crc = fw_calculate_checksum(fw_offsets[fw_updater_target_segment], fw_sizes[fw_updater_target_segment]);

                if (new_crc == fw_updater_target_crc) {
                    RESET();
                } else {
                    fw_updater_attempt++;

                    if (fw_updater_attempt < FIRMWARE_ATTEMPTS) {
                        fw_updater_state = FIRMWARE_PROCESS_IDLE;
                        fw_updater_start(fw_updater_target_segment, fw_updater_target_version);
                    } else {
                        fw_updater_state = FIRMWARE_PROCESS_FAILED;
                    }
                }
            }
        }
    }
}
