#include <xc.h>
#include <stdint.h>
#include "module.h"
#include "protocol.h"
#include "protocol_network.h"
#include "serial.h"
#include "can.h"

/* Local function prototypes. */
void protocol_network_announcement_receive(uint8_t id, uint8_t size, uint8_t *payload);
void protocol_network_nak_receive(uint8_t id, uint8_t size, uint8_t *payload);

#define OPCODE_NETWORK_ANNOUNCEMENT 0x00
#define OPCODE_NETWORK_NAK          0x01

/**
 * Handle reception of a new packet from CAN that is for the CAN address prefix.
 * 
 * @param id CAN id for the source module, from the 8 least significant bits of the raw 11-bit CAN id
 * @param size size of CAN payload received
 * @param payload pointer to payload
 */
void protocol_network_receive(uint8_t id, uint8_t size, uint8_t *payload) {
    /* Safety check, if size is 0 we can not check the opcode. */
    if (size == 0) {
        return;
    }
    
    /* Switch to the correct packet based on opcode in packet. */
    switch (payload[0]) {
        case OPCODE_NETWORK_ANNOUNCEMENT:
            protocol_network_announcement_receive(id, size, payload);
            break;
        case OPCODE_NETWORK_NAK:
            protocol_network_nak_receive(id, size, payload);
            break;
        default:
            /* Alert an unknown opcode has been received. */
            module_error_raise(MODULE_ERROR_PROTOCOL_UNKNOWN | (PREFIX_MODULE << 8) | payload[0], true);
            break;
    }
}

/*
 * CAN Address - Announcement Packet - (0x00)
 * 
 * Packet Layout:
 * 
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Op Code      |  Serial No.                                   |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Serial No.   |                                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

/**
 * Send a CAN address announcement packet.
 */
void protocol_network_announcement_send(void) {
    uint8_t payload[5];
    
    uint32_t serial = serial_get();
    
    payload[0] = OPCODE_NETWORK_ANNOUNCEMENT;
    payload[1] = (serial >> 24) & 0xff;
    payload[2] = (serial >> 16) & 0xff;
    payload[3] = (serial >> 8) & 0xff;
    payload[4] = serial & 0xff;

    can_send(PREFIX_NETWORK, sizeof(payload), &payload[0]);
}

/**
 * Receive a CAN address announcement packet.
 * 
 * @param id CAN id for the source module, from the 8 least significant bits of the raw 11-bit CAN id
 * @param size size of CAN payload received
 * @param payload pointer to payload
 */
void protocol_network_announcement_receive(uint8_t id, uint8_t size, uint8_t *payload) {    
    /* Safety check, if size is < 4 there is no mode. */
    if (size < 5) {
        return;
    }              

    uint32_t serial = (uint32_t) (((uint32_t) payload[1] << 24) | ((uint32_t)payload[2] << 16) | ((uint32_t)payload[3] << 8) | (uint32_t) payload[4]);
    
    can_address_check(id);
}

/*
 * CAN Address - NAK Packet - (0x01)
 * 
 * Packet Layout:
 * 
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Op Code      |  Serial No.                                   |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Serial No.   |                                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

/**
 * Send a CAN address nak packet.
 */
void protocol_network_nak_send(void) {
    uint8_t payload[5];
    
    uint32_t serial = serial_get();
    
    payload[0] = OPCODE_NETWORK_NAK;
    payload[1] = (serial >> 24) & 0xff;
    payload[2] = (serial >> 16) & 0xff;
    payload[3] = (serial >> 8) & 0xff;
    payload[4] = serial & 0xff;

    can_send(PREFIX_NETWORK, sizeof(payload), &payload[0]);
}

/**
 * Receive a CAN address nak packet.
 * 
 * @param id CAN id for the source module, from the 8 least significant bits of the raw 11-bit CAN id
 * @param size size of CAN payload received
 * @param payload pointer to payload
 */
void protocol_network_nak_receive(uint8_t id, uint8_t size, uint8_t *payload) {    
    /* Safety check, if size is < 4 there is no mode. */
    if (size < 5) {
        return;
    }              

    uint32_t serial = (uint32_t) (payload[1] << 24 | payload[2] << 16 | payload[3] << 8 | payload[4]);
    
    can_address_conflict(id);
}