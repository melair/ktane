#include "edgework_bus/router.h"

bool EdgeworkBusRouter_Dispatch(const EdgeworkBus_Router *router,
                                const uint8_t *data, const size_t length) {
    if ((router == NULL) || (data == NULL) ||
        (length < EDGEWORK_BUS_SIZE_HEADER)) {
        return false;
    }

    const EdgeworkBus_Packet *const packet = (const EdgeworkBus_Packet *) data;
    const EdgeworkBus_OpCode opcode = (EdgeworkBus_OpCode) packet->header.opcode;

#define EDGEWORK_BUS_ROUTER_DISPATCH(opcode_, value, member)                 \
    case opcode_: {                                                         \
        if ((length < SIZE_##opcode_) || (router->member == NULL)) {         \
            return false;                                                   \
        }                                                                   \
        const EdgeworkBus_Message message = {                               \
            .address = packet->header.address,                              \
            .opcode = opcode_,                                              \
            .packet = packet,                                               \
            .length = length,                                               \
        };                                                                  \
        router->member(&message);                                            \
        return true;                                                        \
    }

    switch (opcode) {
        EDGEWORK_BUS_PROTOCOL_PACKETS(EDGEWORK_BUS_ROUTER_DISPATCH)
        default:
            return false;
    }

#undef EDGEWORK_BUS_ROUTER_DISPATCH
}
