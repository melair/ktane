#ifndef BACKPLANE_BUS_H
#define BACKPLANE_BUS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool BackplaneBus_Init(void);

void BackplaneBus_Service(void);

/**
 * Send a packet only on the physical backplane bus.
 *
 * This is for forwarding packets received from a node link; locally generated
 * packets should use BackplaneBus_Send so chassis node links receive them too.
 */
bool BackplaneBus_SendPhysical(const uint8_t *data, size_t length);

void BackplaneBus_ProcessPacket(const uint8_t *data, size_t length);

void BackplaneBus_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif // BACKPLANE_BUS_H
