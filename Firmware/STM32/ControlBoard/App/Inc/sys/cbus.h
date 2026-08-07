#ifndef CBUS_H
#define CBUS_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {

#endif

void CBUS_Init(void);

void CBUS_Service(void (*packetHandler)(uint8_t length, void *data));

void CBUS_Queue(uint8_t length, void *data);

#ifdef __cplusplus
}
#endif

#endif //CBUS_H
