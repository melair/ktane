#ifndef NODES_H
#define NODES_H

#include "protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

void Nodes_ReceiveAnnounce(uint8_t identifier, OpCode opcode, Packet *packet);
void Nodes_Service(void);

#ifdef __cplusplus
}
#endif

#endif //NODES_H
