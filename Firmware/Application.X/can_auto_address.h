#ifndef CAN_AUTO_ADDRESS_H
#define	CAN_AUTO_ADDRESS_H

void can_address_initialise(uint8_t initial_id);
void can_address_service(void);
bool can_address_ready(void);

#include "../common/packet.h"

void can_address_receive_announce(uint8_t id, packet_t *p);
void can_address_receive_nak(uint8_t id, packet_t *p);

#endif	/* CAN_AUTO_ADDRESS_H */

