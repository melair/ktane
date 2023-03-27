#ifndef PROTOCOL_MODULE_H
#define	PROTOCOL_MODULE_H

#include <stdbool.h>

void protocol_module_receive(uint8_t id, uint8_t size, uint8_t *payload);
void protocol_module_announcement_send(void);
void protocol_module_error_send(uint16_t code, bool active);
void protocol_module_reset_send(void);
void protocol_module_identify_send(uint8_t id);
void protocol_module_mode_set_send(uint8_t id, uint8_t mode);
void protocol_module_special_function_send(uint8_t id, uint8_t special_fn);

#endif	/* PROTOCOL_MODULE_H */
