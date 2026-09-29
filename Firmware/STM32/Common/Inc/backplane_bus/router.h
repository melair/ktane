#ifndef BACKPLANE_BUS_ROUTER_H
#define BACKPLANE_BUS_ROUTER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "backplane_bus/protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t address;
    BackplaneBus_OpCode opcode;
    const BackplaneBus_Packet *packet;
    size_t length;
} BackplaneBus_Message;

typedef void (*BackplaneBus_PacketHandler)(const BackplaneBus_Message *message);

#define BACKPLANE_BUS_ROUTER_DECLARE_HANDLER(opcode, value, member) \
    BackplaneBus_PacketHandler member;

typedef struct {
    BACKPLANE_BUS_PROTOCOL_PACKETS(BACKPLANE_BUS_ROUTER_DECLARE_HANDLER)
} BackplaneBus_Router;

#undef BACKPLANE_BUS_ROUTER_DECLARE_HANDLER

/**
 * Validate and dispatch a decoded backplane bus packet.
 *
 * The packet pointer in BackplaneBus_Message is only valid for the duration of
 * the handler call.
 *
 * @return true if a registered handler was called; false otherwise.
 */
bool BackplaneBusRouter_Dispatch(const BackplaneBus_Router *router,
                               const uint8_t *data, size_t length);

#ifdef __cplusplus
}
#endif

#endif // BACKPLANE_BUS_ROUTER_H
