#ifndef CAN_H
#define CAN_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {

#endif

typedef enum {
    CAN_DIRECTION_IN = 0,
    CAN_DIRECTION_OUT,
} CAN_Direction;

typedef union {
    uint32_t queueToSofUs;
    uint32_t sofToProcessingUs;
} CAN_Timing;

typedef struct {
    uint16_t identifier;
    uint8_t length;
    void *data;

    CAN_Direction direction;
    CAN_Timing timing;
} CAN_Packet;

typedef void (*CAN_PacketHandler)(const CAN_Packet *packet);

void CAN_Init(void);

void CAN_Service(CAN_PacketHandler packetHandler);

void CAN_Queue(uint16_t identifier, uint8_t length, void *data);

#ifdef __cplusplus
}
#endif

#endif //CAN_H
