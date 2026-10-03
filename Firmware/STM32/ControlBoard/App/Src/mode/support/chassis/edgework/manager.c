#include "mode/support/chassis/edgework/manager.h"

#include "edgework/bus/protocol.h"
#include "mode/support/chassis/edgework/bus.h"
#include "mode.h"

#include <string.h>

#define EDGEWORK_SEND_INTERVAL_MS 25U
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
    edgework->next_inquiry_ms = edgework->next_send_ms;
    edgework->eor_received = false;
    return Bus_Init();
}

void Edgework_StatusReceive(const EdgeworkBus_Message *message) {
    if (message->address >= EDGEWORK_SLOT_COUNT) {
        return;
    }

    if (message->packet->header.flags.eor) {
        edgework->eor_received = true;
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

static bool edgework_queue_inquiry(void) {
    EdgeworkBus_Packet packet = {0};
    packet.header.address = edgework->next_slot;
    packet.header.opcode = EDGEWORK_BUS_INQUIRY;

    if (!Edgework_QueueCommand(&packet, SIZE_EDGEWORK_BUS_INQUIRY)) {
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

    if (time_reached(now_ms, edgework->next_inquiry_ms) &&
        edgework_queue_inquiry()) {
        edgework->next_inquiry_ms = now_ms + EDGEWORK_INQUIRY_INTERVAL_MS;
    }

    if (!edgework->eor_received && !time_reached(now_ms, edgework->next_send_ms)) {
        return;
    }

    if ((edgework->command_count > 0U) && edgework_send_queued_command()) {
        edgework->eor_received = false;
        edgework->next_send_ms = now_ms + EDGEWORK_SEND_INTERVAL_MS;
    }
}
