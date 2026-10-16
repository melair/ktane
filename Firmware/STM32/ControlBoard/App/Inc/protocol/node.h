#ifndef PROTOCOL_NODE_H
#define PROTOCOL_NODE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Announce or reject the CAN identifier currently selected by Protocol. */
void Protocol_NodeIdentifierAnnounce(uint32_t serial);
void Protocol_NodeIdentifierNak(uint32_t serial);

/* Send the module's current status; uptime is in milliseconds. */
void Protocol_NodeAnnounce(uint32_t serial, uint32_t uptime, uint8_t mode,
                           uint8_t state, uint8_t chassis_location);

#ifdef __cplusplus
}
#endif

#endif // PROTOCOL_NODE_H
