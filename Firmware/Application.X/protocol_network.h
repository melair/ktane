#ifndef PROTOCOL_NETWORK_H
#define	PROTOCOL_NETWORK_H

void protocol_network_receive(uint8_t id, uint8_t size, uint8_t *payload);
void protocol_network_address_announce_send(void);
void protocol_network_address_nak_send(void);
void protocol_network_domain_announce_send(uint8_t domain);

#endif	/* PROTOCOL_NETWORK_H */

