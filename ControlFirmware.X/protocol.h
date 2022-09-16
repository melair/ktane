#ifndef PROTOCOL_H
#define	PROTOCOL_H

void protocol_receive(uint8_t prefix, uint8_t id, uint8_t size, uint8_t *payload);

#define PREFIX_MODULE       0b000
#define PREFIX_GAME         0b010

#define PREFIX_CAN_ADDRESS  0b100

#define PREFIX_FIRMWARE     0b110
#define PREFIX_DEBUG        0b111

#endif	/* PROTOCOL_H */

