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

bool NodeLink_ForwardBackplanePacket(const uint8_t *data, size_t length);

void NodeLink_Front_IRQHandler(void);

void NodeLink_Rear_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif // NODE_LINK_H
