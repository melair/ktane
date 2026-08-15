#ifndef NODES_H
#define NODES_H

#include "protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

void Nodes_ReceiveAnnounce(Protocol_Message *message);
void Nodes_Service(void);

#ifdef __cplusplus
}
#endif

#endif //NODES_H
