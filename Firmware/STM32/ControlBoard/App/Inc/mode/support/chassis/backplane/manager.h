#ifndef CHASSIS_BACKPLANE_MANAGER_H
#define CHASSIS_BACKPLANE_MANAGER_H

#include "backplane_bus/router.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BACKPLANE_COUNT 7U
#define BACKPLANE_COMMAND_QUEUE_SIZE 16U
#define BACKPLANE_INQUIRY_INTERVAL_MS 30U
#define BACKPLANE_CHASSIS_ADDRESS 6U
#define BACKPLANE_DEFAULT_POWER_BUDGET_MA 3000U

typedef enum {
    BACKPLANE_PORT_FRONT = 0,
    BACKPLANE_PORT_REAR,
    BACKPLANE_PORT_COUNT,
} Backplane_Port;

typedef enum {
    BACKPLANE_POWER_ABSENT = 0,
    BACKPLANE_POWER_WAITING,
    BACKPLANE_POWER_ENABLE_PENDING,
    BACKPLANE_POWER_POWERED,
    BACKPLANE_POWER_SHED_REQUESTED,
    BACKPLANE_POWER_SHED_PENDING,
    BACKPLANE_POWER_ADMIN_DISABLE_REQUESTED,
    BACKPLANE_POWER_ADMIN_DISABLE_PENDING,
    BACKPLANE_POWER_ADMIN_DISABLED,
    BACKPLANE_POWER_TRIPPED,
} Backplane_PowerState;

typedef struct {
    uint16_t current_milliamps;
    uint16_t peak_current_milliamps;
    uint8_t current_limit_deciamps;
    Power_State channel_state;
    Backplane_PowerState management_state;
    /* Includes startup reservations and power awaiting confirmed shutdown. */
    uint16_t allocated_milliamps;
    /* Power state machine retry deadline, advanced on successful enqueue. */
    uint32_t next_power_retry_ms;
    struct {
        unsigned module_detected :1;
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
    uint32_t next_inquiry_ms;
    bool eor_received;
    uint32_t power_budget_milliamps;
    Backplane_Command command_queue[BACKPLANE_COMMAND_QUEUE_SIZE];
    Backplane_StateData backplanes[BACKPLANE_COUNT];
} Backplane_Data;

bool Backplane_Init(void);

void Backplane_Service(void);

/** Set the allocation budget; excess allocations are shed by the service loop. */
void Backplane_SetPowerBudget(uint32_t milliamps);

/**
 * Set administrative intent for a module port (addresses 0-5).
 * Disable persists across module replacement until enabled or reinitialized.
 * Enable obeys the budget. True means accepted, not confirmed by hardware.
 */
bool Backplane_SetModuleEnabled(uint8_t address, Backplane_Port port, bool enabled);

/**
 * Copy an inquiry or current-limit request into the command FIFO.
 * Enable requests instead set administrative intent through SetModuleEnabled.
 * Returns false for an invalid request or a full FIFO for queued requests.
 * Telemetry is updated only by received status.
 */
bool Backplane_QueueCommand(const BackplaneBus_Packet *packet, size_t length);

void Backplane_StatusReceive(const BackplaneBus_Message *message);

#ifdef __cplusplus
}
#endif

#endif // CHASSIS_BACKPLANE_MANAGER_H
