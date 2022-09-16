#ifndef PROTOCOL_NETWORK_H
#define	PROTOCOL_NETWORK_H

void protocol_network_receive(uint8_t id, uint8_t size, uint8_t *payload);
void protocol_network_announcement_send(void);
void protocol_network_nak_send(void);

#endif	/* PROTOCOL_NETWORK_H */

