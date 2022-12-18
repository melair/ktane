#ifndef CAN_H
#define	CAN_H

#include <stdbool.h>

#define CAN_NO_ADDRESS 0xff

void can_initialise(void);
void can_send(uint8_t prefix, uint8_t size, uint8_t *data);
void can_service(void);
uint8_t can_get_id(void);
bool can_ready(void);
void can_address_conflict(uint8_t id);
void can_address_check(uint8_t id);
uint8_t can_get_domain(void);
void can_domain_update(uint8_t domain);

#endif	/* CAN_H */

