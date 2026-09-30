#include "mode/support/chassis/backplane/manager.h"

#include "mode.h"
#include "mode/support/chassis/backplane/bus.h"
#include "stm32h5xx_hal.h"

#include <string.h>

#define BACKPLANE_POLL_INTERVAL_MS 25U
#define BACKPLANE_STATUS_TIMEOUT_MS 1500U

static Backplane_Data *const backplane = &mode_data.mode.chassis.backplane;

static bool time_reached(const uint32_t now_ms, const uint32_t deadline_ms) {
    return (int32_t) (now_ms - deadline_ms) >= 0;
}

bool Backplane_Init(void) {
    backplane->next_send_ms = HAL_GetTick();
    return true;
}

void Backplane_StatusReceive(const BackplaneBus_Message *message) {
    if (message->address >= BACKPLANE_COUNT) {
        return;
    }

    Backplane_StateData *const board = &backplane->backplanes[message->address];
    const Backplane_Port channel = message->packet->status.flags._front_rear
        ? BACKPLANE_PORT_REAR : BACKPLANE_PORT_FRONT;
    Backplane_PortData *const port = &board->ports[channel];
    port->flags.module_detected = message->packet->status.flags.module_detected;
    port->flags.active = message->packet->status.flags.enabled;
    port->flags.tripped = message->packet->status.flags.tripped;
    port->current_milliamps = message->packet->status.current_milliamps;
    port->current_limit_deciamps = message->packet->status.current_limit_deciamps;
    board->last_ms = HAL_GetTick();
    board->flags.awake = true;
}

bool Backplane_QueueCommand(const BackplaneBus_Packet *packet, const size_t length) {
    if ((packet == NULL) || (length < BACKPLANE_BUS_SIZE_HEADER) ||
        (length > sizeof(*packet)) || (packet->header.address >= BACKPLANE_COUNT) ||
        (backplane->command_count >= BACKPLANE_COMMAND_QUEUE_SIZE)) {
        return false;
    }

    size_t expected_length;
    switch (packet->header.opcode) {
        case BACKPLANE_BUS_INQUIRY:
            expected_length = SIZE_BACKPLANE_BUS_INQUIRY;
            break;
        case BACKPLANE_BUS_SET_ENABLED:
            expected_length = SIZE_BACKPLANE_BUS_SET_ENABLED;
            break;
        case BACKPLANE_BUS_SET_CURRENT_LIMIT:
            expected_length = SIZE_BACKPLANE_BUS_SET_CURRENT_LIMIT;
            break;
        default:
            return false;
    }
    if (length != expected_length) {
        return false;
    }

    Backplane_Command *const command = &backplane->command_queue[backplane->command_write];
    memcpy(&command->packet, packet, length);
    command->length = length;
    backplane->command_write =
        (uint8_t) ((backplane->command_write + 1U) % BACKPLANE_COMMAND_QUEUE_SIZE);
    backplane->command_count++;
    return true;
}

static bool backplane_send_queued_command(void) {
    Backplane_Command *const command = &backplane->command_queue[backplane->command_read];
    if (!BackplaneBus_Send(&command->packet, command->length)) {
        return false;
    }

    backplane->command_read =
        (uint8_t) ((backplane->command_read + 1U) % BACKPLANE_COMMAND_QUEUE_SIZE);
    backplane->command_count--;
    return true;
}

static bool backplane_send_inquiry(void) {
    BackplaneBus_Packet packet = {0};
    packet.header.address = backplane->next_backplane;
    packet.header.opcode = BACKPLANE_BUS_INQUIRY;
    if (!BackplaneBus_Send(&packet, SIZE_BACKPLANE_BUS_INQUIRY)) {
        return false;
    }

    backplane->next_backplane =
        (uint8_t) ((backplane->next_backplane + 1U) % BACKPLANE_COUNT);
    return true;
}

void Backplane_Service(void) {
    const uint32_t now_ms = HAL_GetTick();
    for (size_t index = 0U; index < BACKPLANE_COUNT; ++index) {
        Backplane_StateData *const board = &backplane->backplanes[index];
        if (board->flags.awake &&
            time_reached(now_ms, board->last_ms + BACKPLANE_STATUS_TIMEOUT_MS)) {
            board->flags.awake = false;
        }
    }

    if (!time_reached(now_ms, backplane->next_send_ms)) {
        return;
    }

    const bool sent = (backplane->command_count > 0U)
        ? backplane_send_queued_command() : backplane_send_inquiry();
    if (sent) {
        backplane->next_send_ms = now_ms + BACKPLANE_POLL_INTERVAL_MS;
    }
}
