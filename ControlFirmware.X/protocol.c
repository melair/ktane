#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include "protocol.h"
#include "protocol_module.h"
#include "can.h"
#include "mode.h"
#include "tick.h"
#include "modules.h"
#include "buzzer.h"
#include "argb.h"

/**
 * Handled reception of a new packet from CAN.
 * 
 * @param prefix protocol prefix from the 3 most significant bits of the raw 11-bit CAN id
 * @param id CAN id for the source module, from the 8 least significant bits of the raw 11-bit CAN id
 * @param size size of CAN payload received
 * @param payload pointer to payload
 */
void protocol_receive(uint8_t prefix, uint8_t id, uint8_t size, uint8_t *payload) {
    /* Detect a CAN ID conflict, report as an error. This ignores the packet, 
     * this might result in a desync of the game - but it's a critical error
     * and we can not continue.*/
    if (id == can_get_id()) {
        module_error_raise(MODULE_ERROR_CAN_ID_CONFLICT);
        return;
    }   
    
    /* Switch to correct set of op codes based on protocol prefix. */
    switch (prefix) {
        case PREFIX_MODULE:
            protocol_module_receive(id, size, payload);
            break;
        default:
            /* Alert an unknown prefix has been received. */
            module_error_raise(MODULE_ERROR_PROTOCOL_UNKNOWN_PREFIX);
            break;
    }
}