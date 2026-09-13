#ifndef EDGEWORK_MANAGER_H
#define EDGEWORK_MANAGER_H

#include <stdbool.h>
#include <stdint.h>

#include "edgework/bus/router.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EDGEWORK_SLOT_COUNT 14U
#define EDGEWORK_COMMAND_QUEUE_SIZE 16U

typedef struct {
    uint32_t last_ms;
    uint8_t mode;
    uint8_t state;

    struct {
        unsigned active :1;
        unsigned identifying :1;
    } flags;
} Edgework_SlotData;

typedef struct {
    EdgeworkBus_Packet packet;
    size_t length;
} Edgework_Command;

typedef struct {
    uint8_t next_slot;
    uint8_t command_read;
    uint8_t command_write;
    uint8_t command_count;
    uint32_t next_send_ms;
    Edgework_Command command_queue[EDGEWORK_COMMAND_QUEUE_SIZE];
    Edgework_SlotData slots[EDGEWORK_SLOT_COUNT];
} Edgework_Data;

bool Edgework_Init(void);

void Edgework_Service(void);

/**
 * Add an outbound edgework command to the FIFO.
 *
 * The packet is copied, so it may be stack allocated by the caller. Returns
 * false when the packet is invalid or the queue is full.
 */
bool Edgework_QueueCommand(const EdgeworkBus_Packet *packet, size_t length);

void Edgework_StatusReceive(const EdgeworkBus_Message *message);

#ifdef __cplusplus
}
#endif

#endif // EDGEWORK_MANAGER_H
