#include "mode.h"

#include "nvm/nvm.h"

#define MODE_NVM_ID 0x0000U

typedef struct {
    EdgeworkMode active;
} Mode_State;

static Mode_State mode = {
    .active = MODE_UNKNOWN,
};

bool Mode_Init(void) {
    const NVM_Query query = {
        .type = UINT8,
        .id = MODE_NVM_ID,
        .data = &mode.active,
    };

    return NVM_Read(&query, 1U);
}

EdgeworkMode Mode_Get(void) {
    return mode.active;
}

bool Mode_Set(const EdgeworkMode new_mode) {
    EdgeworkMode stored_mode = new_mode;
    const NVM_Query query = {
        .type = UINT8,
        .id = MODE_NVM_ID,
        .data = &stored_mode,
    };

    if (!NVM_Write(&query)) {
        return false;
    }

    mode.active = new_mode;
    return true;
}
