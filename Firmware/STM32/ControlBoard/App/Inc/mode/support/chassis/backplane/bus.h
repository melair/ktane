#ifndef CHASSIS_BACKPLANE_BUS_H
#define CHASSIS_BACKPLANE_BUS_H

#include "backplane_bus/protocol.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool BackplaneBus_Receive(const uint8_t *data, size_t length);

bool BackplaneBus_Send(const BackplaneBus_Packet *packet, size_t length);

#ifdef __cplusplus
}
#endif

#endif // CHASSIS_BACKPLANE_BUS_H
