#ifndef NODES_H
#define NODES_H

#include "protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

void Nodes_ReceiveAnnounce(Protocol_Message *message);
void Nodes_Service(void);

/* Returns NODE_CHASSIS_LOCATION_UNKNOWN when the sender is missing or inactive. */
uint8_t Nodes_GetChassisLocation(uint8_t identifier);

#ifdef __cplusplus
}
#endif

#endif //NODES_H
