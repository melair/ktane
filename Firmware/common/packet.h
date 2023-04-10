#ifndef PACKET_H
#define	PACKET_H

#include "protocol.h"

void packet_register(uint8_t prefix, uint8_t opcode, void (*fn)(void));
void packet_route(uint8_t src, uint8_t prefix, packet_t *packet);
void packet_send(uint8_t prefix, packet_t *packet);

extern packet_t packet_outgoing;
extern packet_t *packet_incomming;
extern uint8_t packet_incomming_id;

#endif	/* PACKET_H */

