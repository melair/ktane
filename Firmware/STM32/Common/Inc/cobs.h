#ifndef COBS_H
#define COBS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define COBS_PACKET_MAX_SIZE 64U
#define COBS_ENCODED_MAX_SIZE (COBS_PACKET_MAX_SIZE + (COBS_PACKET_MAX_SIZE / 254U) + 1U)
#define COBS_FRAME_MAX_SIZE (COBS_ENCODED_MAX_SIZE + 1U)

typedef void (*COBS_PacketHandler)(const uint8_t *data, size_t length);

typedef struct {
    bool rx_synced;
    size_t rx_length;
    uint8_t rx_buffer[COBS_ENCODED_MAX_SIZE];
    uint8_t decoded_buffer[COBS_PACKET_MAX_SIZE];
    COBS_PacketHandler packet_handler;
} COBS_State;

void COBS_Init(COBS_State *cobs, COBS_PacketHandler packet_handler);

void COBS_Service(COBS_State *cobs, const uint8_t *data, size_t length);

bool COBS_Encode(const uint8_t *data, size_t length,
                 uint8_t *frame, size_t frame_capacity, size_t *frame_length);

#ifdef __cplusplus
}
#endif

#endif //COBS_H
