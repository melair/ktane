#include "node_link/router.h"

bool NodeLinkRouter_Dispatch(const NodeLink_Router *router,
                             const uint8_t *data, const size_t length) {
    if ((router == NULL) || (data == NULL) ||
        (length < NODE_LINK_SIZE_HEADER)) {
        return false;
    }

    const NodeLink_Packet *const packet = (const NodeLink_Packet *) data;
    const NodeLink_OpCode opcode = (NodeLink_OpCode) packet->header.opcode;

#define NODE_LINK_ROUTER_DISPATCH(opcode_, value, member)                    \
    case opcode_: {                                                         \
        if ((length < SIZE_##opcode_) || (router->member == NULL)) {         \
            return false;                                                   \
        }                                                                   \
        const NodeLink_Message message = {                                  \
            .opcode = opcode_,                                              \
            .packet = packet,                                               \
            .length = length,                                               \
        };                                                                  \
        router->member(&message);                                            \
        return true;                                                        \
    }

    switch (opcode) {
        NODE_LINK_PROTOCOL_PACKETS(NODE_LINK_ROUTER_DISPATCH)
        default:
            return false;
    }

#undef NODE_LINK_ROUTER_DISPATCH
}
