#ifndef CAN_H
#define CAN_H

#include "pin.h"

#define CAN_ID_UNKNOWN 0xff

void can_init(pin_t tx, pin_t rx, pin_t act, void (*callback)(uint16_t can_id, uint8_t len, uint8_t *ptr));
void can_tx(uint16_t can_id, uint8_t len, uint8_t *ptr);
void can_service(void);
void can_interrupt(void); 

#endif