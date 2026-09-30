#ifndef CHASSIS_BACKPLANE_MANAGER_H
#define CHASSIS_BACKPLANE_MANAGER_H

#include "backplane_bus/router.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BACKPLANE_COUNT 7U
#define BACKPLANE_COMMAND_QUEUE_SIZE 16U

typedef enum {
    BACKPLANE_PORT_FRONT = 0,
    BACKPLANE_PORT_REAR,
    BACKPLANE_PORT_COUNT,
} Backplane_Port;

typedef struct {
    uint16_t current_milliamps;
    uint8_t current_limit_deciamps;
    struct {
        unsigned module_detected :1;
        unsigned active :1;
        unsigned tripped :1;
    } flags;
} Backplane_PortData;

typedef struct {
    uint32_t last_ms;
    /* Telemetry remains last-known when awake expires. */
    struct {
        unsigned awake :1;
    } flags;
    Backplane_PortData ports[BACKPLANE_PORT_COUNT];
} Backplane_StateData;

typedef struct {
    BackplaneBus_Packet packet;
    size_t length;
} Backplane_Command;

typedef struct {
    uint8_t next_backplane;
    uint8_t command_read;
    uint8_t command_write;
    uint8_t command_count;
    uint32_t next_send_ms;
    Backplane_Command command_queue[BACKPLANE_COMMAND_QUEUE_SIZE];
    Backplane_StateData backplanes[BACKPLANE_COUNT];
} Backplane_Data;

bool Backplane_Init(void);

void Backplane_Service(void);

/**
 * Copy an inquiry, enable, or current-limit request into the command FIFO.
 * Returns false for an invalid packet/address/length or a full queue.
 * State is updated only by received status, not by enqueuing commands.
 */
bool Backplane_QueueCommand(const BackplaneBus_Packet *packet, size_t length);

void Backplane_StatusReceive(const BackplaneBus_Message *message);

#ifdef __cplusplus
}
#endif

#endif // CHASSIS_BACKPLANE_MANAGER_H
