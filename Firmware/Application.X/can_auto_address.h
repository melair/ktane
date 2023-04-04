#ifndef CAN_AUTO_ADDRESS_H
#define	CAN_AUTO_ADDRESS_H

void can_address_initialise(uint8_t initial_id);
void can_address_service(void);
bool can_address_ready(void);

#endif	/* CAN_AUTO_ADDRESS_H */

