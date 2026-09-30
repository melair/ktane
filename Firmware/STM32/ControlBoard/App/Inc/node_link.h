#ifndef NODE_LINK_H
#define NODE_LINK_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool NodeLink_Init(void);

void NodeLink_Service(void);

/** Encapsulate and enqueue a backplane packet; false on invalid input or full TX queue. */
bool NodeLink_SendBackplanePacket(const uint8_t *data, size_t length);

#ifdef __cplusplus
}
#endif

#endif // NODE_LINK_H
