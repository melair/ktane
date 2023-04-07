#include <xc.h>
#include <stdint.h>
#include "protocol.h"
#include "can.h"
#include "packet.h"

packet_t packet_outgoing;

typedef struct {
    uint8_t prefix;
    uint8_t opcode;
    void (*fn)(uint8_t, packet_t *);
} packet_route_t;

uint8_t packet_routes_used = 0;
packet_route_t packet_routes[OPCODE_COUNT];

/* This is a very poor implementation of a router, it may seek through all
 * entries before finding a match, or failing to. This would be better
 * served with a tree. Or at the very least arrays per prefix type. */

void packet_route(uint8_t src, uint8_t prefix, packet_t *packet) {
    /* Detect a CAN ID conflict, report as an error. This ignores the packet,
     * this might result in a desync of the game - but it's a critical error
     * and we can not continue. We have to allow conflicts in the NETWORK
     * prefix otherwise we can't use autoassignment. */
    if (prefix != PREFIX_NETWORK && src == can_get_id()) {
        // TODO: Raise can conflict error.
        // module_error_raise(MODULE_ERROR_CAN_ID_CONFLICT, true);
        return;
    }

    for (uint8_t i = 0; i < packet_routes_used; i++) {
        if (packet_routes[i].prefix == prefix && packet_routes[i].opcode == packet->opcode) {
            packet_routes[i].fn(src, packet);
            return;
        }
    }

    // TODO: Raise error for no packet handler.
    // module_error_raise(MODULE_ERROR_PROTOCOL_UNKNOWN | (PREFIX_FIRMWARE << 8) | payload[0], true);
}

void packet_register(uint8_t prefix, uint8_t opcode, void (*fn)(uint8_t, packet_t *)) {
    packet_routes[packet_routes_used].prefix = prefix;
    packet_routes[packet_routes_used].opcode = opcode;
    packet_routes[packet_routes_used].fn = fn;
    packet_routes_used++;
}

void packet_send(uint8_t prefix, packet_t *packet) {
    uint8_t size = 0xff;

    for (uint8_t i = 0; i < OPCODE_COUNT; i++) {
        if (protocol_sizes[i].prefix == prefix && protocol_sizes[i].opcode == packet->opcode) {
            size = protocol_sizes[i].size;
            break;
        }
    }

    if (size != 0xff) {
        can_send(prefix, size, (uint8_t *) packet);
    }
}