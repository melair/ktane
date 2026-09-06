#include "backplane.h"

#include "nvm/nvm.h"

#define BACKPLANE_NVM_LOCATION_ID 0x0000U

typedef struct {
    BackplaneLocation location;
} Backplane_State;

static Backplane_State backplane = {
    .location = BACKPLANE_LOCATION_UNKNOWN,
};

bool Backplane_Init(void) {
    const NVM_Query query = {
        .type = UINT8,
        .id = BACKPLANE_NVM_LOCATION_ID,
        .data = &backplane.location,
    };

    if (!NVM_Read(&query, 1U)) {
        return false;
    }

    return true;
}

BackplaneLocation Backplane_GetLocation(void) {
    return backplane.location;
}

bool Backplane_SetLocation(const BackplaneLocation location) {
    BackplaneLocation stored_location = location;
    const NVM_Query query = {
        .type = UINT8,
        .id = BACKPLANE_NVM_LOCATION_ID,
        .data = &stored_location,
    };

    if (!NVM_Write(&query)) {
        return false;
    }

    backplane.location = location;
    return true;
}
