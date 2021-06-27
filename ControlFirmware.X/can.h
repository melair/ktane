#ifndef CAN_H
#define	CAN_H

void can_initialise(void);
void can_send(uint8_t prefix, uint8_t size, uint8_t *data);
void can_service(void);
uint8_t can_get_id(void);

#endif	/* CAN_H */

