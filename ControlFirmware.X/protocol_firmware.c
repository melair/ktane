#include <xc.h>
#include <stdint.h>
#include "modules.h"
#include "protocol.h"
#include "protocol_firmware.h"
#include "firmware.h"
#include "mode.h"
#include "can.h"

/* Local function prototypes. */
void protocol_firmware_request_receive(uint8_t id, uint8_t size, uint8_t *payload);
void protocol_firmware_header_receive(uint8_t id, uint8_t size, uint8_t *payload);
void protocol_firmware_header_send(void);
void protocol_firmware_page_request_receive(uint8_t id, uint8_t size, uint8_t *payload);
void protocol_firmware_page_response_receive(uint8_t id, uint8_t size, uint8_t *payload);
void protocol_firmware_page_response_send(uint16_t page);

#define OPCODE_FIRMWARE_REQUEST         0x00
#define OPCODE_FIRMWARE_HEADER          0x01
#define OPCODE_FIRMWARE_PAGE_REQUEST    0x02
#define OPCODE_FIRMWARE_PAGE_RESPONSE            0x03

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
            module_error_raise(MODULE_ERROR_PROTOCOL_UNKNOWN_OPCODE);
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
   |  Op Code      |  Firmware Version             |               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

/**
 * Send a firmware request.
 */
void protocol_firmware_request_send(uint16_t requested_version) {
    uint8_t payload[3];
        
    payload[0] = OPCODE_FIRMWARE_REQUEST;
    payload[1] = (requested_version >> 8) & 0xff;
    payload[2] = requested_version & 0xff;

    can_send(PREFIX_FIRMWARE, 3, &payload[0]);
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
    if (size < 3) {
        return;
    }              
     
    uint16_t fw = (uint16_t) ((payload[1] << 8) | payload[2]);
    
    if (mode_get() != MODE_CONTROLLER || firmware_get_version() != fw) {
        return;
    }

    protocol_firmware_header_send();
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
   |  CRC-32       |                                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

/**
 * Send a firmware header
 */
void protocol_firmware_header_send(void) {
    uint8_t payload[9];
    
    uint16_t fw = firmware_get_version();
    uint16_t pages = firmware_get_pages();
    uint32_t crc = firmware_get_checksum();
    
    payload[0] = OPCODE_FIRMWARE_HEADER;
    payload[1] = (fw >> 8) & 0xff;
    payload[2] = fw & 0xff;
    payload[3] = (pages >> 8) & 0xff;
    payload[4] = pages & 0xff;
    payload[5] = (crc >> 24) & 0xff;
    payload[6] = (crc >> 16) & 0xff;
    payload[7] = (crc >> 8) & 0xff;
    payload[8] = crc & 0xff;
        
    can_send(PREFIX_FIRMWARE, 9, &payload[0]);
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
    if (size < 9) {
        return;
    }              
     
    uint16_t fw = (uint16_t) ((payload[1] << 8) | payload[2]);
    uint16_t pages = (uint16_t) ((payload[3] << 8) | payload[4]);
    uint8_t a = payload[5];
    uint8_t b = payload[6];
    uint8_t c = payload[7];
    uint8_t d = payload[8];
    
    uint32_t crc = ((uint32_t) a << 24) | ((uint32_t)b << 16) | ((uint32_t) c << 8) | ((uint32_t) d);
    
    firmware_header_received(id, fw, pages, crc);
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
 */

/**
 * Send a firmware page request.
 */
void protocol_firmware_page_request_send(uint16_t page, uint8_t source_id) {
    uint8_t payload[4];
        
    payload[0] = OPCODE_FIRMWARE_PAGE_REQUEST;
    payload[1] = (page >> 8) & 0xff;
    payload[2] = page & 0xff;
    payload[3] = source_id;

    can_send(PREFIX_FIRMWARE, 4, &payload[0]);
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
    if (size < 4) {
        return;
    }              
     
    uint16_t page = (uint16_t) ((payload[1] << 8) | payload[2]);
    uint8_t source_id = payload[3];
    
    if (source_id != can_get_id()) {
        return;
    }
    
    protocol_firmware_page_response_send(page);
}

/*
 * Firmware - Page Response Packet - (0x03)
 * 
 * Packet Layout:
 * 
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Op Code      |  Firmware Page                | UNUSED        |
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
void protocol_firmware_page_response_send(uint16_t page) {
    uint8_t payload[20];
        
    payload[0] = OPCODE_FIRMWARE_PAGE_RESPONSE;
    payload[1] = (page >> 8) & 0xff;
    payload[2] = page & 0xff;

    firmware_get_page(page, &payload[4]);
        
    can_send(PREFIX_FIRMWARE, 20, &payload[0]);
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
    
    firmware_page_received(id, page, &payload[4]);
}
