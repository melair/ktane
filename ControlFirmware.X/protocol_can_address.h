#ifndef PROTOCOL_CAN_ADDRESS_H
#define	PROTOCOL_CAN_ADDRESS_H

void protocol_can_address_receive(uint8_t id, uint8_t size, uint8_t *payload);
void protocol_can_address_announcement_send(void);
void protocol_can_address_nak_send(void);

#endif	/* PROTOCOL_CAN_ADDRESS_H */

