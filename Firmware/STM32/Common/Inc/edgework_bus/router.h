#ifndef EDGEWORK_BUS_ROUTER_H
#define EDGEWORK_BUS_ROUTER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "edgework_bus/protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t address;
    EdgeworkBus_OpCode opcode;
    const EdgeworkBus_Packet *packet;
    size_t length;
} EdgeworkBus_Message;

typedef void (*EdgeworkBus_PacketHandler)(const EdgeworkBus_Message *message);

#define EDGEWORK_BUS_ROUTER_DECLARE_HANDLER(opcode, value, member) \
    EdgeworkBus_PacketHandler member;

typedef struct {
    EDGEWORK_BUS_PROTOCOL_PACKETS(EDGEWORK_BUS_ROUTER_DECLARE_HANDLER)
} EdgeworkBus_Router;

#undef EDGEWORK_BUS_ROUTER_DECLARE_HANDLER

/**
 * Validate and dispatch a decoded edgework bus packet.
 *
 * The packet pointer in EdgeworkBus_Message is only valid for the duration of
 * the handler call.
 *
 * @return true if a registered handler was called; false otherwise.
 */
bool EdgeworkBusRouter_Dispatch(const EdgeworkBus_Router *router,
                                const uint8_t *data, size_t length);

#ifdef __cplusplus
}
#endif

#endif // EDGEWORK_BUS_ROUTER_H
