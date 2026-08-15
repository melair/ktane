#include "nodes.h"

#include <stdbool.h>
#include <stdint.h>
#include "stm32h5xx_hal.h"

#define MAXIMUM_NODES 16
#define NODE_INACTIVE_TIMEOUT_MS 5000U

typedef struct {
    uint8_t identifier;
    uint8_t mode;

    uint32_t serial;
    uint32_t uptime;

    uint8_t chassis_location;

    uint32_t last_announcement;

    struct {
        bool active;
    } flags;
} Node;

static Node nodes[MAXIMUM_NODES] = {0};

void Nodes_ReceiveAnnounce(Protocol_Message *message) {
    const uint32_t now = HAL_GetTick();
    Node *node = NULL;

    for (uint8_t index = 0U; index < MAXIMUM_NODES; index++) {
        if (nodes[index].identifier == message->identifier) {
            node = &nodes[index];
            break;
        }

        if (!nodes[index].flags.active &&
            ((node == NULL) ||
             (nodes[index].last_announcement < node->last_announcement))) {
            node = &nodes[index];
        }
    }

    if (node == NULL) {
        return;
    }

    *node = (Node) {
        .identifier = message->identifier,
        .mode = message->packet->module.announce.mode,
        .serial = message->packet->module.announce.serial,
        .uptime = message->packet->module.announce.uptime,
        .chassis_location = message->packet->module.announce.flags.chassis_location,
        .last_announcement = now,
        .flags = {
            .active = true,
        },
    };
}

void Nodes_Service(void) {
    const uint32_t now = HAL_GetTick();

    for (uint8_t index = 0U; index < MAXIMUM_NODES; index++) {
        if (nodes[index].flags.active &&
            ((now - nodes[index].last_announcement) >= NODE_INACTIVE_TIMEOUT_MS)) {
            nodes[index].flags.active = false;
        }
    }
}
