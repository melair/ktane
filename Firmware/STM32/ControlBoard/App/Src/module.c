#include "module.h"
#include <stdint.h>
#include "stm32h5xx_hal.h"
#include "mode.h"
#include "sys/nvm.h"
#include "sys/fsm.h"
#include "module_fsm.h"
#include "mode/puzzle/simon/simon.h"
#include "mode/support/chassis/chassis.h"
#include "mode/support/timer/timer.h"

typedef struct {
    uint8_t mode;
    FSM fsm;
    Mode_Definition *definition;
    bool service_enabled;
} module_t;

static module_t module = {
    .mode = MODE_NONE,
    .definition = NULL,
    .service_enabled = false,
};

Module_Data module_data = {0};

void Module_Init(void) {
    module.mode = MODE_NONE;

    const NVM_Query query = {
        .type = UINT8,
        .id = MODE_CFG(MODE_NONE, MODE_CONFIG_ACTIVE_MODE),
        .data = &module.mode,
    };

    NVM_Read(&query, 1);

    module.definition = NULL;
    module.service_enabled = false;

    switch (module.mode) {
        case MODE_PUZZLE_SIMON:
            module.definition = &simon_mode;
            break;
        case MODE_SUPPORT_CHASSIS:
            module.definition = &chassis_mode;
            break;
        case MODE_SUPPORT_TIMER:
            module.definition = &timer_mode;
            break;
        default:
            break;
    }

    FSM_Init(&module.fsm, module_fsm_states, MODULE_FSM_STATE_INIT, module.definition);
}

void Module_Service(void) {
    if (module.service_enabled &&
        (module.definition != NULL) &&
        (module.definition->always_service != NULL)) {
        module.definition->always_service();
    }

    FSM_Service(&module.fsm);
}

void Module_SetServiceEnabled(const bool enabled) {
    module.service_enabled = enabled;
}

void Module_Set(const uint8_t mode) {
    module.mode = mode;

    const NVM_Query query = {
        .type = UINT8,
        .id = MODE_CFG(MODE_NONE, MODE_CONFIG_ACTIVE_MODE),
        .data = &module.mode,
    };

    if (NVM_Write(&query)) {
        NVIC_SystemReset();
    }
}
