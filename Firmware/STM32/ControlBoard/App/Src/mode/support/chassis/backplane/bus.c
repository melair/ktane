#include "mode/support/chassis/backplane/bus.h"

#include "backplane_bus/router.h"
#include "mode.h"
#include "mode/support/chassis/backplane/manager.h"
#include "node_link.h"

static const BackplaneBus_Router backplane_bus_router = {
    .status = Backplane_StatusReceive,
};

bool BackplaneBus_Receive(const uint8_t *data, const size_t length) {
    if (Mode_Get() != MODE_SUPPORT_CHASSIS) {
        return false;
    }

    return BackplaneBusRouter_Dispatch(&backplane_bus_router, data, length);
}

bool BackplaneBus_Send(const BackplaneBus_Packet *packet, const size_t length) {
    if (Mode_Get() != MODE_SUPPORT_CHASSIS) {
        return false;
    }

    return NodeLink_SendBackplanePacket((const uint8_t *) packet, length);
}
