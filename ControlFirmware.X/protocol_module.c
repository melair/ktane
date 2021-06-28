#include <xc.h>
#include <stdint.h>
#include "modules.h"
#include "protocol.h"
#include "protocol_module.h"
#include "firmware.h"
#include "mode.h"
#include "can.h"

/* Local function prototypes. */
void protocol_module_announcement_receive(uint8_t id, uint8_t size, uint8_t *payload);
void protocol_module_error_receive(uint8_t id, uint8_t size, uint8_t *payload);

#define OPCODE_MODULE_ANNOUNCEMENT 0x00
#define OPCODE_MODULE_ERROR 0xf0

/**
 * Handle reception of a new packet from CAN that is for the module management
 * prefix.
 * 
 * @param id CAN id for the source module, from the 8 least significant bits of the raw 11-bit CAN id
 * @param size size of CAN payload received
 * @param payload pointer to payload
 */
void protocol_module_receive(uint8_t id, uint8_t size, uint8_t *payload) {
    /* Safety check, if size is 0 we can not check the opcode. */
    if (size == 0) {
        return;
    }
    
    /* Switch to the correct packet based on opcode in packet. */
    switch (payload[0]) {
        case OPCODE_MODULE_ANNOUNCEMENT:
            protocol_module_announcement_receive(id, size, payload);
            break;
        case OPCODE_MODULE_ERROR:
            protocol_module_error_receive(id, size, payload);
            break;
        default:
            /* Alert an unknown opcode has been received. */
            module_error_raise(MODULE_ERROR_PROTOCOL_UNKNOWN_OPCODE);
            break;
    }
}

/*
 * Module - Announcement Packet - (0x00)
 * 
 * Packet Layout:
 * 
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Op Code      |  Module Mode  |  Firmware Version             |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

/**
 * Send a module announcement packet.
 */
void protocol_module_announcement_send(void) {
    uint8_t payload[4];
    
    uint16_t fw = firmware_get_version();
    
    payload[0] = OPCODE_MODULE_ANNOUNCEMENT;
    payload[1] = mode_get();
    payload[2] = (fw >> 8) & 0xff;
    payload[3] = fw & 0xff;
    
    can_send(PREFIX_MODULE, 4, &payload[0]);
}

/**
 * Receive a module announcement packet
 * 
 * @param id CAN id for the source module, from the 8 least significant bits of the raw 11-bit CAN id
 * @param size size of CAN payload received
 * @param payload pointer to payload
 */
void protocol_module_announcement_receive(uint8_t id, uint8_t size, uint8_t *payload) {    
    /* Safety check, if size is < 4 there is no mode. */
    if (size < 4) {
        return;
    }              

    uint8_t mode = payload[1];    
    uint16_t fw = (uint16_t) ((payload[2] << 8) | payload[3]);

    module_seen(id, mode, fw);  
    
    if (mode == MODE_CONTROLLER) {
        firmware_check(fw);
    }
}

/*
 * Module - Error Announcement - (0xf0)
 * 
 * Packet Layout:
 * 
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Op Code      |  Error Code                   |               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

/**
 * Announce an error has occurred on this module to the CAN network.
 * 
 * @param code error code that has occurred.
 */
void protocol_module_error_send(uint16_t code) {
    uint8_t payload[3];
    
    payload[0] = OPCODE_MODULE_ERROR;
    payload[1] = (code >> 8) & 0xff;
    payload[2] = code & 0xff;
    
    can_send(PREFIX_MODULE, 3, &payload[0]);    
}

/**
 * Receive a module error packet, update module database with error.
 * 
 * @param id CAN id for the source module, from the 8 least significant bits of the raw 11-bit CAN id
 * @param size size of CAN payload received
 * @param payload pointer to payload
 */
void protocol_module_error_receive(uint8_t id, uint8_t size, uint8_t *payload) {    
    /* Safety check, if size is < 3 there is no error code. */
    if (size < 3) {
        return;
    }
    
    uint16_t code = (uint16_t) ((payload[1] << 8) | payload[2]);    
    module_error_record(id, code);
}

