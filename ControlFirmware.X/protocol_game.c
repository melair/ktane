#include <xc.h>
#include <stdint.h>
#include "modules.h"
#include "protocol_game.h"

/* Local function prototypes. */

#define OPCODE_GAME_         0x00

/**
 * Handle reception of a new packet from CAN that is for the game management
 * prefix.
 * 
 * @param id CAN id for the source module, from the 8 least significant bits of the raw 11-bit CAN id
 * @param size size of CAN payload received
 * @param payload pointer to payload
 */
void protocol_game_receive(uint8_t id, uint8_t size, uint8_t *payload) {
    /* Safety check, if size is 0 we can not check the opcode. */
    if (size == 0) {
        return;
    }
    
    /* Switch to the correct packet based on opcode in packet. */
    switch (payload[0]) {
        default:
            /* Alert an unknown opcode has been received. */
            module_error_raise(MODULE_ERROR_PROTOCOL_UNKNOWN_OPCODE);
            break;
    }
}
