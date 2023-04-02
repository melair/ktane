#include <xc.h>
#include <stdint.h>
#include "module.h"
#include "protocol.h"
#include "protocol_firmware.h"
#include "can.h"
#include "../common/fw.h"
#include "../common/segments.h"

/* Local function prototypes. */
void protocol_firmware_request_receive(uint8_t id, uint8_t size, uint8_t *payload);
void protocol_firmware_header_receive(uint8_t id, uint8_t size, uint8_t *payload);
void protocol_firmware_header_send(uint8_t segment);
void protocol_firmware_page_request_receive(uint8_t id, uint8_t size, uint8_t *payload);
void protocol_firmware_page_response_receive(uint8_t id, uint8_t size, uint8_t *payload);
void protocol_firmware_page_response_send(uint16_t page, uint8_t segment);

#define OPCODE_FIRMWARE_REQUEST         0x00
#define OPCODE_FIRMWARE_HEADER          0x01
#define OPCODE_FIRMWARE_PAGE_REQUEST    0x02
#define OPCODE_FIRMWARE_PAGE_RESPONSE   0x03

/**
 * Handle reception of a new packet from CAN that is for the firmware management
 * prefix.
 *
 * @param id CAN id for the source module, from the 8 least significant bits of the raw 11-bit CAN id
 * @param size size of CAN payload received
 * @param payload pointer to payload
 */
void protocol_firmware_receive(uint8_t id, uint8_t size, uint8_t *payload) {
    /* Safety check, if size is 0 we can not check the opcode. */
    if (size == 0) {
        return;
    }

    /* Switch to the correct packet based on opcode in packet. */
    switch (payload[0]) {
        case OPCODE_FIRMWARE_REQUEST:
            protocol_firmware_request_receive(id, size, payload);
            break;
        case OPCODE_FIRMWARE_HEADER:
            protocol_firmware_header_receive(id, size, payload);
            break;
        case OPCODE_FIRMWARE_PAGE_REQUEST:
            protocol_firmware_page_request_receive(id, size, payload);
            break;
        case OPCODE_FIRMWARE_PAGE_RESPONSE:
            protocol_firmware_page_response_receive(id, size, payload);
            break;
        default:
            /* Alert an unknown opcode has been received. */
            module_error_raise(MODULE_ERROR_PROTOCOL_UNKNOWN | (PREFIX_FIRMWARE << 8) | payload[0], true);
            break;
    }
}

/*
 * Firmware - Request Packet - (0x00)
 *
 * Packet Layout:
 *
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Op Code      |  Firmware Version             |  Segment      |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

/**
 * Send a firmware request.
 */
void protocol_firmware_request_send(uint16_t requested_version, uint8_t segment) {
    uint8_t payload[4];

    payload[0] = OPCODE_FIRMWARE_REQUEST;
    payload[1] = (requested_version >> 8) & 0xff;
    payload[2] = requested_version & 0xff;
    payload[3] = segment;

    can_send(PREFIX_FIRMWARE, sizeof(payload), &payload[0]);
}

/**
 * Receive a firmware request packet, this function has logic in it that
 * will automatically send a head in response if the module is a controller and
 * the firmware version matches.
 *
 * @param id CAN id for the source module, from the 8 least significant bits of the raw 11-bit CAN id
 * @param size size of CAN payload received
 * @param payload pointer to payload
 */
void protocol_firmware_request_receive(uint8_t id, uint8_t size, uint8_t *payload) {   
    /* Safety check. */
    if (size < 4) {
        return;
    }

    uint16_t fw = (uint16_t) ((payload[1] << 8) | payload[2]);
    uint8_t segment = payload[3];
    
    if (segment >= SEGMENT_COUNT) {
        return;
    }

    if (fw_version(segment) != fw) {
        return;
    }

#ifndef __DEBUG
    protocol_firmware_header_send(segment);
#endif
}

/*
 * Firmware - Header Packet - (0x01)
 *
 * Packet Layout:
 *
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Op Code      |  Firmware Version             | Pages         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Pages        |  CRC-32                                       |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  CRC-32       |  Segment      |                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

/**
 * Send a firmware header
 */
void protocol_firmware_header_send(uint8_t segment) {
    uint8_t payload[10];

    uint16_t fw = fw_version(segment);
    uint16_t pages = fw_page_count(segment);
    uint32_t crc = fw_checksum(segment);

    payload[0] = OPCODE_FIRMWARE_HEADER;
    payload[1] = (fw >> 8) & 0xff;
    payload[2] = fw & 0xff;
    payload[3] = (pages >> 8) & 0xff;
    payload[4] = pages & 0xff;
    payload[5] = (crc >> 24) & 0xff;
    payload[6] = (crc >> 16) & 0xff;
    payload[7] = (crc >> 8) & 0xff;
    payload[8] = crc & 0xff;
    payload[9] = segment;

    can_send(PREFIX_FIRMWARE, sizeof(payload), &payload[0]);
}

/**
 * Receive a firmware header packet.
 *
 * @param id CAN id for the source module, from the 8 least significant bits of the raw 11-bit CAN id
 * @param size size of CAN payload received
 * @param payload pointer to payload
 */
void protocol_firmware_header_receive(uint8_t id, uint8_t size, uint8_t *payload) {
    /* Safety check. */
    if (size < 10) {
        return;
    }

    uint16_t fw = (uint16_t) ((payload[1] << 8) | payload[2]);
    uint16_t pages = (uint16_t) ((payload[3] << 8) | payload[4]);
    uint8_t a = payload[5];
    uint8_t b = payload[6];
    uint8_t c = payload[7];
    uint8_t d = payload[8];
    uint8_t segment = payload[9];

    uint32_t crc = ((uint32_t) a << 24) | ((uint32_t)b << 16) | ((uint32_t) c << 8) | ((uint32_t) d);

    // TODO: Receive firmware header!   
}

/*
 * Firmware - Page Request Packet - (0x02)
 *
 * Packet Layout:
 *
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Op Code      |  Firmware Page                |  Source ID    |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Segment      |                                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

/**
 * Send a firmware page request.
 */
void protocol_firmware_page_request_send(uint16_t page, uint8_t source_id, uint8_t segment) {
    uint8_t payload[5];

    payload[0] = OPCODE_FIRMWARE_PAGE_REQUEST;
    payload[1] = (page >> 8) & 0xff;
    payload[2] = page & 0xff;
    payload[3] = source_id;
    payload[4] = segment;

    can_send(PREFIX_FIRMWARE, sizeof(payload), &payload[0]);
}

/**
 * Receive a firmware page request packet.
 *
 * @param id CAN id for the source module, from the 8 least significant bits of the raw 11-bit CAN id
 * @param size size of CAN payload received
 * @param payload pointer to payload
 */
void protocol_firmware_page_request_receive(uint8_t id, uint8_t size, uint8_t *payload) {
    /* Safety check. */
    if (size < 5) {
        return;
    }

    uint16_t page = (uint16_t) ((payload[1] << 8) | payload[2]);
    uint8_t source_id = payload[3];
    uint8_t segment = payload[4];
    
    if (segment >= SEGMENT_COUNT) {
        return;
    }

    if (source_id != can_get_id()) {
        return;
    }

    protocol_firmware_page_response_send(page, segment);
}

/*
 * Firmware - Page Response Packet - (0x03)
 *
 * Packet Layout:
 *
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Op Code      |  Firmware Page                |  Segment      |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Firmware Page Data                                           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Firmware Page Data                                           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Firmware Page Data                                           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Firmware Page Data                                           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

/**
 * Send a firmware page.
 */
void protocol_firmware_page_response_send(uint16_t page, uint8_t segment) {
    uint8_t payload[20];

    payload[0] = OPCODE_FIRMWARE_PAGE_RESPONSE;
    payload[1] = (page >> 8) & 0xff;
    payload[2] = page & 0xff;
    payload[3] = segment;

    fw_page_read(segment, page, &payload[4]);

    can_send(PREFIX_FIRMWARE, sizeof(payload), &payload[0]);
}

/**
 * Receive a firmware page packet.
 *
 * @param id CAN id for the source module, from the 8 least significant bits of the raw 11-bit CAN id
 * @param size size of CAN payload received
 * @param payload pointer to payload
 */
void protocol_firmware_page_response_receive(uint8_t id, uint8_t size, uint8_t *payload) {
    /* Safety check. */
    if (size < 20) {
        return;
    }

    uint16_t page = (uint16_t) ((payload[1] << 8) | payload[2]);
    uint8_t segment = payload[3];
    
    if (segment >= SEGMENT_COUNT) {
        return;
    }

    // TODO: Receive firmware page!   
}
