#ifndef CAN_H
#define CAN_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {

#endif

void CAN_Init(void);

void CAN_Service(void (*packetHandler)(uint16_t mailbox, uint8_t length, void *data));

void CAN_Queue(uint16_t mailbox, uint8_t length, void *data);

#ifdef __cplusplus
}
#endif

#endif //CAN_H
