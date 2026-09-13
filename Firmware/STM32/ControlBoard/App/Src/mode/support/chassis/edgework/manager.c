#include "mode/support/chassis/edgework/manager.h"

#include "edgework/bus/protocol.h"
#include "mode/support/chassis/edgework/bus.h"
#include "mode.h"

#include <string.h>

#define EDGEWORK_POLL_INTERVAL_MS 25U
#define EDGEWORK_STATUS_TIMEOUT_MS 1500U

static Edgework_Data *const edgework = &mode_data.mode.chassis.edgework;

static bool time_reached(const uint32_t now_ms, const uint32_t deadline_ms) {
    return (int32_t) (now_ms - deadline_ms) >= 0;
}

bool Edgework_Init(void) {
    edgework->next_slot = 0U;
    edgework->command_read = 0U;
    edgework->command_write = 0U;
    edgework->command_count = 0U;
    edgework->next_send_ms = HAL_GetTick();
    return Bus_Init();
}

void Edgework_StatusReceive(const EdgeworkBus_Message *message) {
    if (message->address >= EDGEWORK_SLOT_COUNT) {
        return;
    }

    Edgework_SlotData *const slot = &edgework->slots[message->address];
    slot->mode = message->packet->status.mode;
    slot->state = message->packet->status.state;
    slot->last_ms = HAL_GetTick();
    slot->flags.active = true;
    slot->flags.identifying = message->packet->status.data.identify;
}

bool Edgework_QueueCommand(const EdgeworkBus_Packet *packet, const size_t length) {
    if ((packet == NULL) || (length < EDGEWORK_BUS_SIZE_HEADER) ||
        (length > sizeof(*packet)) ||
        (edgework->command_count >= EDGEWORK_COMMAND_QUEUE_SIZE)) {
        return false;
    }

    Edgework_Command *const command =
        &edgework->command_queue[edgework->command_write];
    memcpy(&command->packet, packet, length);
    command->length = length;

    edgework->command_write =
        (uint8_t) ((edgework->command_write + 1U) % EDGEWORK_COMMAND_QUEUE_SIZE);
    edgework->command_count++;
    return true;
}

static bool edgework_send_queued_command(void) {
    Edgework_Command *const command =
        &edgework->command_queue[edgework->command_read];
    if (!Bus_Send(&command->packet, command->length)) {
        return false;
    }

    edgework->command_read =
        (uint8_t) ((edgework->command_read + 1U) % EDGEWORK_COMMAND_QUEUE_SIZE);
    edgework->command_count--;
    return true;
}

static bool edgework_send_inquiry(void) {
    EdgeworkBus_Packet packet = {0};
    packet.header.address = edgework->next_slot;
    packet.header.opcode = EDGEWORK_BUS_INQUIRY;

    if (!Bus_Send(&packet, SIZE_EDGEWORK_BUS_INQUIRY)) {
        return false;
    }

    edgework->next_slot = (uint8_t) ((edgework->next_slot + 1U) % EDGEWORK_SLOT_COUNT);
    return true;
}

void Edgework_Service(void) {
    Bus_Service();

    const uint32_t now_ms = HAL_GetTick();
    for (size_t slot = 0U; slot < EDGEWORK_SLOT_COUNT; ++slot) {
        Edgework_SlotData *const slot_data = &edgework->slots[slot];
        if (slot_data->flags.active &&
            time_reached(now_ms, slot_data->last_ms + EDGEWORK_STATUS_TIMEOUT_MS)) {
            slot_data->flags.active = false;
        }
    }

    if (!time_reached(now_ms, edgework->next_send_ms)) {
        return;
    }

    const bool sent = (edgework->command_count > 0U)
        ? edgework_send_queued_command()
        : edgework_send_inquiry();
    if (sent) {
        edgework->next_send_ms = now_ms + EDGEWORK_POLL_INTERVAL_MS;
    }
}
