#include "chassis_bus/router.h"

bool ChassisBusRouter_Dispatch(const ChassisBus_Router *router,
                               const uint8_t *data, const size_t length) {
    if ((router == NULL) || (data == NULL) ||
        (length < CHASSIS_BUS_SIZE_HEADER)) {
        return false;
    }

    const ChassisBus_Packet *const packet = (const ChassisBus_Packet *) data;
    const ChassisBus_OpCode opcode = (ChassisBus_OpCode) packet->header.opcode;

#define CHASSIS_BUS_ROUTER_DISPATCH(opcode_, value, member)                  \
    case opcode_: {                                                         \
        if ((length < SIZE_##opcode_) || (router->member == NULL)) {         \
            return false;                                                   \
        }                                                                   \
        const ChassisBus_Message message = {                                \
            .address = packet->header.address,                              \
            .opcode = opcode_,                                              \
            .packet = packet,                                               \
            .length = length,                                               \
        };                                                                  \
        router->member(&message);                                            \
        return true;                                                        \
    }

    switch (opcode) {
        CHASSIS_BUS_PROTOCOL_PACKETS(CHASSIS_BUS_ROUTER_DISPATCH)
        default:
            return false;
    }

#undef CHASSIS_BUS_ROUTER_DISPATCH
}
