#ifndef PROTOCOL_H
#define	PROTOCOL_H

void protocol_receive(uint8_t prefix, uint8_t id, uint8_t size, uint8_t *payload);

void protocol_module_announcement_send(void);
void protocol_module_error_send(uint16_t code);

#define PREFIX_MODULE 0b000
#define PREFIX_GAME   0b010
#define PREFIX_DEBUG  0b111

#endif	/* PROTOCOL_H */

