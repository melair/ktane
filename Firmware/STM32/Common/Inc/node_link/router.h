#ifndef NODE_LINK_ROUTER_H
#define NODE_LINK_ROUTER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "node_link/protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    NodeLink_OpCode opcode;
    const NodeLink_Packet *packet;
    size_t length;
} NodeLink_Message;

typedef void (*NodeLink_PacketHandler)(const NodeLink_Message *message);

#define NODE_LINK_ROUTER_DECLARE_HANDLER(opcode, value, member) \
    NodeLink_PacketHandler member;

typedef struct {
    NODE_LINK_PROTOCOL_PACKETS(NODE_LINK_ROUTER_DECLARE_HANDLER)
} NodeLink_Router;

#undef NODE_LINK_ROUTER_DECLARE_HANDLER

/**
 * Validate and dispatch a decoded node link packet.
 *
 * The packet pointer in NodeLink_Message is only valid for the duration of the
 * handler call.
 *
 * @return true if a registered handler was called; false otherwise.
 */
bool NodeLinkRouter_Dispatch(const NodeLink_Router *router,
                             const uint8_t *data, size_t length);

#ifdef __cplusplus
}
#endif

#endif // NODE_LINK_ROUTER_H
