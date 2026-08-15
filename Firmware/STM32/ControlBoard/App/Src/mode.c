#include "mode.h"
#include <stdint.h>
#include "protocol.h"
#include "stm32h5xx_hal.h"
#include "sys/fsm.h"
#include "sys/mcu_init.h"
#include "sys/nvm.h"
#include "sys/tick.h"
#include "mode_fsm.h"
#include "mode/puzzle/simon/simon.h"
#include "mode/support/chassis/chassis.h"
#include "mode/support/timer/timer.h"

typedef struct {
    uint8_t mode;
    FSM fsm;
    Mode_Definition *definition;
    bool service_enabled;
} mode_t;

static mode_t mode = {
    .mode = MODE_NONE,
    .definition = NULL,
    .service_enabled = false,
};

Mode_Data mode_data = {0};

void Mode_Init(void) {
    mode.mode = MODE_NONE;

    const NVM_Query query = {
        .type = UINT8,
        .id = MODE_CFG(MODE_NONE, MODE_CONFIG_ACTIVE_MODE),
        .data = &mode.mode,
    };

    NVM_Read(&query, 1);

    mode.definition = NULL;
    mode.service_enabled = false;

    switch (mode.mode) {
        case MODE_PUZZLE_SIMON:
            mode.definition = &simon_mode;
            break;
        case MODE_SUPPORT_CHASSIS:
            mode.definition = &chassis_mode;
            break;
        case MODE_SUPPORT_TIMER:
            mode.definition = &timer_mode;
            break;
        default:
            break;
    }

    FSM_Init(&mode.fsm, mode_fsm_states, MODE_FSM_STATE_INIT, mode.definition);
}

void Mode_Service(void) {
    if (mode.service_enabled &&
        (mode.definition != NULL) &&
        (mode.definition->always_service != NULL)) {
        mode.definition->always_service();
    }

    FSM_Service(&mode.fsm);

    if (tick_2hz) {
        Packet announcement = {0};
        announcement.node.announce.serial = UID;
        announcement.node.announce.uptime = HAL_GetTick();
        announcement.node.announce.mode = mode.mode;
        announcement.node.announce.state = mode.fsm.current_id;
        announcement.node.announce.flags.chassis_location = NODE_CHASSIS_LOCATION_UNKNOWN;
        Protocol_Send(MODULE_ANNOUNCE, &announcement);
    }
}

void Mode_SetServiceEnabled(const bool enabled) {
    mode.service_enabled = enabled;
}

void Mode_Set(const uint8_t new_mode) {
    mode.mode = new_mode;

    const NVM_Query query = {
        .type = UINT8,
        .id = MODE_CFG(MODE_NONE, MODE_CONFIG_ACTIVE_MODE),
        .data = &mode.mode,
    };

    if (NVM_Write(&query)) {
        NVIC_SystemReset();
    }
}
