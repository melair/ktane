#include <xc.h>
#include <stdint.h>
#include "module.h"
#include "protocol.h"
#include "protocol_module.h"
#include "mode.h"
#include "can.h"
#include "serial.h"
#include "status.h"
#include "../common/nvm.h"
#include "../common/eeprom_addrs.h"
#include "../common/fw.h"
#include "../common/segments.h"

/* Local function prototypes. */
void protocol_module_announcement_receive(uint8_t id, uint8_t size, uint8_t *payload);
void protocol_module_error_receive(uint8_t id, uint8_t size, uint8_t *payload);
void protocol_module_reset_receive(uint8_t id, uint8_t size, uint8_t *payload);
void protocol_module_identify_receive(uint8_t id, uint8_t size, uint8_t *payload);
void protocol_module_mode_set_receive(uint8_t id, uint8_t size, uint8_t *payload);
void protocol_module_special_function_receive(uint8_t id, uint8_t size, uint8_t *payload);

#define OPCODE_MODULE_ANNOUNCEMENT      0x00
#define OPCODE_MODULE_RESET             0x10
#define OPCODE_MODULE_IDENTIFY          0x11
#define OPCODE_MODULE_MODE_SET          0x12
#define OPCODE_MODULE_SPECIAL_FUNCTION  0x13
#define OPCODE_MODULE_ERROR             0xf0

/* Store if we need to announce our reset. */
bool announce_reset = true;

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
        case OPCODE_MODULE_RESET:
            protocol_module_reset_receive(id, size, payload);
            break;
        case OPCODE_MODULE_IDENTIFY:
            protocol_module_identify_receive(id, size, payload);
            break;
        case OPCODE_MODULE_MODE_SET:
            protocol_module_mode_set_receive(id, size, payload);
            break;
        case OPCODE_MODULE_SPECIAL_FUNCTION:
            protocol_module_special_function_receive(id, size, payload);
            break;
        default:
            /* Alert an unknown opcode has been received. */
            module_error_raise(MODULE_ERROR_PROTOCOL_UNKNOWN | (PREFIX_MODULE << 8) | payload[0], true);
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
   |  Op Code      |  Module Mode  |  Application Version          |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |R|D| | | | | | |  Serial                                       |
   |S|B| | | | | | |                                               |
   |T|G| | | | | | |                                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Serial       |  Bootloader Version           |  Flasher Ver  |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Flasher Ver  |                                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

/**
 * Send a module announcement packet.
 */
void protocol_module_announcement_send(void) {
    uint8_t payload[14];

    uint16_t app_fw = fw_version(APPLICATION);
    uint16_t bootloader_fw = fw_version(BOOTLOADER);
    uint16_t flasher_fw = fw_version(FLASHER);
    
    uint32_t serial = serial_get();
    uint8_t domain = can_get_domain();

    payload[0] = OPCODE_MODULE_ANNOUNCEMENT;
    payload[1] = mode_get();
    payload[2] = (app_fw >> 8) & 0xff;
    payload[3] = app_fw & 0xff;
    payload[4] = 0x00;
    payload[5] = (serial >> 24) & 0xff;
    payload[6] = (serial >> 16) & 0xff;
    payload[7] = (serial >> 8) & 0xff;
    payload[8] = serial & 0xff;
    payload[9] = domain;
    payload[10] = (bootloader_fw >> 8) & 0xff;
    payload[11] = bootloader_fw & 0xff;
    payload[12] = (flasher_fw >> 8) & 0xff;
    payload[13] = flasher_fw & 0xff;
    
    if (announce_reset) {
        announce_reset = false;
        payload[4] |= 0b10000000;
    }
    
#ifdef __DEBUG
    payload[4] |= 0b01000000;    
#endif
    can_send(PREFIX_MODULE, sizeof(payload), &payload[0]);
}

/**
 * Receive a module announcement packet
 *
 * @param id CAN id for the source module, from the 8 least significant bits of the raw 11-bit CAN id
 * @param size size of CAN payload received
 * @param payload pointer to payload
 */
void protocol_module_announcement_receive(uint8_t id, uint8_t size, uint8_t *payload) {
    /* Safety check, if size is < 10 there is no mode. */
    if (size < 14) {
        return;
    }

    uint8_t mode = payload[1];
    uint16_t app_fw = (uint16_t) ((payload[2] << 8) | payload[3]);
    uint16_t bootloader_fw = (uint16_t) ((payload[10] << 8) | payload[11]);
    uint16_t flasher_fw = (uint16_t) ((payload[12] << 8) | payload[13]);
    bool reset = (payload[4] & 0b10000000);
    bool debug = (payload[4] & 0b01000000);
    uint32_t serial = (uint32_t) (((uint32_t) payload[5] << 24) | ((uint32_t) payload[6] << 16) | ((uint32_t) payload[7] << 8) | (uint32_t) payload[8]);
    uint8_t domain = payload[9];

    if (reset) {
        module_errors_clear(id);
    }
    
    module_seen(id, mode, app_fw, serial, domain, debug, bootloader_fw, flasher_fw);     
}

/*
 * Module - Error Announcement - (0xf0)
 *
 * Packet Layout:
 *
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Op Code      |  Error Code                   |A| | | | | | | |
   |               |                               |C| | | | | | | |
   |               |                               |T| | | | | | | |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

/**
 * Announce an error has occurred on this module to the CAN network.
 *
 * @param code error code that has occurred.
 */
void protocol_module_error_send(uint16_t code, bool active) {
    uint8_t payload[4];

    payload[0] = OPCODE_MODULE_ERROR;
    payload[1] = (code >> 8) & 0xff;
    payload[2] = code & 0xff;
    payload[3] = 0x00;

    if (active == true) {
        payload[3] |= 0b10000000;
    }

    can_send(PREFIX_MODULE, sizeof(payload), &payload[0]);
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
    if (size < 4) {
        return;
    }

    uint16_t code = (uint16_t) ((payload[1] << 8) | payload[2]);
    bool active = (payload[3] & 0b10000000) != 0;
    module_error_record(id, code, active);
}

/*
 * Module - Reset - (0x10)
 *
 * Packet Layout:
 *
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Op Code      |                                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

/**
 * Inform the network it is to reset.
 *
 * @param code error code that has occurred.
 */
void protocol_module_reset_send(void) {
    uint8_t payload[1];

    payload[0] = OPCODE_MODULE_RESET;

    can_send(PREFIX_MODULE, 1, &payload[0]);

    /* Wait until the CAN TX buffer is empty, i.e. the above packet has gone. */
    while(!C1TXQSTALbits.TXQEIF);
    /* Reset the MCU. */
    RESET();
}

/**
 * Receive a module reset packet, reset the MCU.
 *
 * @param id CAN id for the source module, from the 8 least significant bits of the raw 11-bit CAN id
 * @param size size of CAN payload received
 * @param payload pointer to payload
 */
void protocol_module_reset_receive(uint8_t id, uint8_t size, uint8_t *payload) {
    RESET();
}

/*
 * Module - Identify - (0x11)
 *
 * Packet Layout:
 *
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Op Code      |  CAN Id       |                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

/**
 * Notify a module that it should flash to identify it.
 *
 * @param id can id to flash
 */
void protocol_module_identify_send(uint8_t id) {
    uint8_t payload[2];

    payload[0] = OPCODE_MODULE_IDENTIFY;
    payload[1] = id;

    can_send(PREFIX_MODULE, sizeof(payload), &payload[0]);

    /* Send packet to self. */
    protocol_module_identify_receive(0, sizeof(payload), &payload[0]);
}

/**
 * Receive a identify reset packet, turn on/off identify status effect.
 *
 * @param id CAN id for the source module, from the 8 least significant bits of the raw 11-bit CAN id
 * @param size size of CAN payload received
 * @param payload pointer to payload
 */
void protocol_module_identify_receive(uint8_t id, uint8_t size, uint8_t *payload) {
    /* Safety check. */
    if (size < 2) {
        return;
    }

    status_identify(payload[1] == can_get_id());
}

/*
 * Module - Mode Set - (0x12)
 *
 * Packet Layout:
 *
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Op Code      |  CAN Id       |  Mode         |               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

/**
 * Notify a module that it should change mode
 *
 * @param id can id to set mode on
 * @param mode new mode to set
 */
void protocol_module_mode_set_send(uint8_t id, uint8_t mode) {
    uint8_t payload[3];

    payload[0] = OPCODE_MODULE_MODE_SET;
    payload[1] = id;
    payload[2] = mode;

    can_send(PREFIX_MODULE, sizeof(payload), &payload[0]);

    /* Send packet to self. */
    protocol_module_identify_receive(0, sizeof(payload), &payload[0]);
}

/**
 * Receive a mode that it should change its mode
 *
 * @param id CAN id for the source module, from the 8 least significant bits of the raw 11-bit CAN id
 * @param size size of CAN payload received
 * @param payload pointer to payload
 */
void protocol_module_mode_set_receive(uint8_t id, uint8_t size, uint8_t *payload) {
    /* Safety check. */
    if (size < 3) {
        return;
    }

    /* If targeted at this module. */
    if (payload[1] == can_get_id()) {
        /* Change module mode in EEPROM. */
        nvm_eeprom_write(EEPROM_LOC_MODE_CONFIGURATION, payload[2]);

        /* Reset the MCU. */
        RESET();
    }
}

/*
 * Module - Special Function - (0x13)
 *
 * Packet Layout:
 *
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Op Code      |  CAN Id       |  Special Fn   |               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

/**
 * Notify a module that a special function has been activated.
 *
 * @param id can id to set mode on
 * @param special_fn special function to activate
 */
void protocol_module_special_function_send(uint8_t id, uint8_t special_fn) {
    uint8_t payload[3];

    payload[0] = OPCODE_MODULE_SPECIAL_FUNCTION;
    payload[1] = id;
    payload[2] = special_fn;

    can_send(PREFIX_MODULE, sizeof(payload), &payload[0]);

    /* Send packet to self. */
    protocol_module_special_function_receive(id, sizeof(payload), &payload[0]);
}

/**
 * Receive a special function message to a module
 *
 * @param id CAN id for the source module, from the 8 least significant bits of the raw 11-bit CAN id
 * @param size size of CAN payload received
 * @param special_fn special function to activate
 */
void protocol_module_special_function_receive(uint8_t id, uint8_t size, uint8_t *payload) {
    /* Safety check. */
    if (size < 3) {
        return;
    }

    uint8_t can_id = payload[1];
    uint8_t fn = payload[2];
    
    /* If targeted at this module. */
    if (can_id == can_get_id()) {
        mode_call_special_function(fn);        
    }
}
