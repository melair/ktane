#ifndef PROTOCOL_MODULE_H
#define	PROTOCOL_MODULE_H

void protocol_module_receive(uint8_t id, uint8_t size, uint8_t *payload);
void protocol_module_announcement_send(void);
void protocol_module_error_send(uint16_t code);
void protocol_module_reset_send(void);

#endif	/* PROTOCOL_MODULE_H */

