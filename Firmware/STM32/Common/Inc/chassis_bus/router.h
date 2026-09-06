#ifndef CHASSIS_BUS_ROUTER_H
#define CHASSIS_BUS_ROUTER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "chassis_bus/protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t address;
    ChassisBus_OpCode opcode;
    const ChassisBus_Packet *packet;
    size_t length;
} ChassisBus_Message;

typedef void (*ChassisBus_PacketHandler)(const ChassisBus_Message *message);

#define CHASSIS_BUS_ROUTER_DECLARE_HANDLER(opcode, value, member) \
    ChassisBus_PacketHandler member;

typedef struct {
    CHASSIS_BUS_PROTOCOL_PACKETS(CHASSIS_BUS_ROUTER_DECLARE_HANDLER)
} ChassisBus_Router;

#undef CHASSIS_BUS_ROUTER_DECLARE_HANDLER

/**
 * Validate and dispatch a decoded chassis bus packet.
 *
 * The packet pointer in ChassisBus_Message is only valid for the duration of
 * the handler call.
 *
 * @return true if a registered handler was called; false otherwise.
 */
bool ChassisBusRouter_Dispatch(const ChassisBus_Router *router,
                               const uint8_t *data, size_t length);

#ifdef __cplusplus
}
#endif

#endif // CHASSIS_BUS_ROUTER_H
