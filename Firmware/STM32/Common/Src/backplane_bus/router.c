#include "backplane_bus/router.h"

bool BackplaneBusRouter_Dispatch(const BackplaneBus_Router *router,
                               const uint8_t *data, const size_t length) {
    if ((router == NULL) || (data == NULL) ||
        (length < BACKPLANE_BUS_SIZE_HEADER)) {
        return false;
    }

    const BackplaneBus_Packet *const packet = (const BackplaneBus_Packet *) data;
    const BackplaneBus_OpCode opcode = (BackplaneBus_OpCode) packet->header.opcode;

#define BACKPLANE_BUS_ROUTER_DISPATCH(opcode_, value, member)                \
    case opcode_: {                                                         \
        if ((length < SIZE_##opcode_) || (router->member == NULL)) {         \
            return false;                                                   \
        }                                                                   \
        const BackplaneBus_Message message = {                              \
            .address = packet->header.address,                              \
            .opcode = opcode_,                                              \
            .packet = packet,                                               \
            .length = length,                                               \
        };                                                                  \
        router->member(&message);                                            \
        return true;                                                        \
    }

    switch (opcode) {
        BACKPLANE_BUS_PROTOCOL_PACKETS(BACKPLANE_BUS_ROUTER_DISPATCH)
        default:
            return false;
    }

#undef BACKPLANE_BUS_ROUTER_DISPATCH
}
