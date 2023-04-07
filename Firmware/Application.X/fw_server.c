#include <xc.h>
#include <stdint.h>
#include "../common/can.h"
#include "../common/segments.h"
#include "../common/packet.h"
#include "../common/protocol.h"
#include "../common/fw.h"

void fw_server_recieve_header_request(uint8_t id, packet_t *p);
void fw_server_recieve_page_request(uint8_t id, packet_t *p);

void fw_server_initialise(void) {
    packet_register(PREFIX_FIRMWARE, OPCODE_FIRMWARE_REQUEST, fw_server_recieve_header_request);
    packet_register(PREFIX_FIRMWARE, OPCODE_FIRMWARE_PAGE_REQUEST, fw_server_recieve_page_request);
}

void fw_server_recieve_header_request(uint8_t id, packet_t *p) {
    if (p->firmware.request.segment < SEGMENT_COUNT && p->firmware.request.version == fw_version(p->firmware.request.segment)) {
        packet_outgoing.opcode = OPCODE_FIRMWARE_HEADER;
        packet_outgoing.firmware.header.version = fw_version(p->firmware.request.segment);
        packet_outgoing.firmware.header.pages = fw_page_count(p->firmware.request.segment);
        packet_outgoing.firmware.header.crc = fw_checksum(p->firmware.request.segment);
        packet_send(PREFIX_FIRMWARE, &packet_outgoing);
    }
}

void fw_server_recieve_page_request(uint8_t id, packet_t *p) {
    if (p->firmware.page_request.segment < SEGMENT_COUNT && p->firmware.page_request.source_id == can_get_id() && p->firmware.page_request.page < fw_page_count(p->firmware.page_request.segment)) {
        packet_outgoing.opcode = OPCODE_FIRMWARE_PAGE_RESPONSE;
        packet_outgoing.firmware.page_response.page = p->firmware.page_request.page;
        packet_outgoing.firmware.page_response.segment = p->firmware.page_request.segment;
        fw_page_read(p->firmware.page_request.segment, p->firmware.page_request.page, &packet_outgoing.firmware.page_response.data[0]);
        packet_send(PREFIX_FIRMWARE, &packet_outgoing);
    }
}
