#include "mode/support/chassis/backplane/manager.h"

#include "mode.h"
#include "mode/support/chassis/backplane/bus.h"
#include "stm32h5xx_hal.h"

#include <string.h>

#define BACKPLANE_POLL_INTERVAL_MS 25U
#define BACKPLANE_BUSY_INQUIRY_INTERVAL_MS 50U
#define BACKPLANE_STATUS_TIMEOUT_MS 1500U
#define BACKPLANE_POWER_RETRY_MS 250U
#define BACKPLANE_STARTUP_ALLOCATION_MA 200U

static Backplane_Data *const backplane = &mode_data.mode.chassis.backplane;

bool Backplane_Init(void) {
    memset(backplane, 0, sizeof(*backplane));
    backplane->power_budget_milliamps = BACKPLANE_DEFAULT_POWER_BUDGET_MA;
    backplane->next_send_ms = HAL_GetTick();
    backplane->next_inquiry_ms = backplane->next_send_ms;
    return true;
}

void Backplane_SetPowerBudget(const uint32_t milliamps) {
    backplane->power_budget_milliamps = milliamps;
    const uint32_t now_ms = HAL_GetTick();
    for (size_t index = 0U; index < BACKPLANE_CHASSIS_ADDRESS; ++index) {
        for (size_t channel = 0U; channel < BACKPLANE_PORT_COUNT; ++channel) {
            Backplane_PortData *const port = &backplane->backplanes[index].ports[channel];
            if (port->management_state == BACKPLANE_POWER_WAITING) {
                port->next_power_retry_ms = now_ms;
            }
        }
    }
}

bool Backplane_SetModuleEnabled(const uint8_t address, const Backplane_Port channel,
                               const bool enabled) {
    if ((address >= BACKPLANE_CHASSIS_ADDRESS) ||
        ((unsigned int) channel >= BACKPLANE_PORT_COUNT)) {
        return false;
    }

    Backplane_PortData *const port = &backplane->backplanes[address].ports[channel];
    const uint32_t now_ms = HAL_GetTick();
    if (!enabled) {
        switch (port->management_state) {
            case BACKPLANE_POWER_ADMIN_DISABLE_REQUESTED:
            case BACKPLANE_POWER_ADMIN_DISABLE_PENDING:
            case BACKPLANE_POWER_ADMIN_DISABLED:
                return true;
            case BACKPLANE_POWER_SHED_PENDING:
                port->management_state = BACKPLANE_POWER_ADMIN_DISABLE_PENDING;
                break;
            default:
                port->management_state = (port->channel_state == POWER_STATE_ACTIVE ||
                                         port->management_state == BACKPLANE_POWER_ENABLE_PENDING ||
                                         port->management_state == BACKPLANE_POWER_SHED_REQUESTED)
                    ? BACKPLANE_POWER_ADMIN_DISABLE_REQUESTED
                    : BACKPLANE_POWER_ADMIN_DISABLED;
                port->next_power_retry_ms = now_ms;
                break;
        }
    } else {
        switch (port->management_state) {
            case BACKPLANE_POWER_ADMIN_DISABLE_REQUESTED:
                /* Cancel an unsent disable unless an enable still needs draining. */
                port->management_state = port->channel_state == POWER_STATE_ACTIVE ? BACKPLANE_POWER_POWERED
                    : (port->allocated_milliamps > 0U ? BACKPLANE_POWER_SHED_REQUESTED
                       : BACKPLANE_POWER_WAITING);
                break;
            case BACKPLANE_POWER_ADMIN_DISABLE_PENDING:
                port->management_state = BACKPLANE_POWER_SHED_PENDING;
                break;
            case BACKPLANE_POWER_ADMIN_DISABLED:
                port->management_state = !port->flags.module_detected ? BACKPLANE_POWER_ABSENT
                    : (port->channel_state == POWER_STATE_TRIPPED ? BACKPLANE_POWER_TRIPPED : BACKPLANE_POWER_WAITING);
                break;
            default:
                break;
        }
        port->next_power_retry_ms = now_ms;
    }
    return true;
}

void Backplane_StatusReceive(const BackplaneBus_Message *message) {
    if (message->address >= BACKPLANE_COUNT) {
        return;
    }

    Backplane_StateData *const board = &backplane->backplanes[message->address];
    const Backplane_Port channel = message->packet->status.flags.port
        ? BACKPLANE_PORT_REAR : BACKPLANE_PORT_FRONT;
    Backplane_PortData *const port = &board->ports[channel];
    const uint16_t previous_allocation = port->allocated_milliamps;
    port->flags.module_detected = message->packet->status.flags.module_detected;
    port->channel_state = (Power_State) message->packet->status.flags.power_state;
    port->current_milliamps = message->packet->status.current_milliamps;
    port->peak_current_milliamps = message->packet->status.peak_current_milliamps;
    port->current_limit_deciamps = message->packet->status.current_limit_deciamps;
    board->last_ms = HAL_GetTick();
    board->flags.awake = true;

    if (port->channel_state == POWER_STATE_ACTIVE) {
        port->allocated_milliamps = (uint16_t) (port->current_limit_deciamps * 100U);
    }
    if (message->address == BACKPLANE_CHASSIS_ADDRESS) {
        port->allocated_milliamps = port->channel_state == POWER_STATE_ACTIVE ? port->allocated_milliamps : 0U;
    } else switch (port->management_state) {
        case BACKPLANE_POWER_ADMIN_DISABLED:
            if (port->channel_state == POWER_STATE_ACTIVE) {
                port->management_state = BACKPLANE_POWER_ADMIN_DISABLE_REQUESTED;
                port->next_power_retry_ms = board->last_ms;
            }
            break;
        case BACKPLANE_POWER_ADMIN_DISABLE_REQUESTED:
        case BACKPLANE_POWER_SHED_REQUESTED:
            /* An off status may precede an in-flight enable; send disable first. */
            break;
        case BACKPLANE_POWER_ADMIN_DISABLE_PENDING:
            if (port->channel_state != POWER_STATE_ACTIVE) {
                port->allocated_milliamps = 0U;
                port->management_state = BACKPLANE_POWER_ADMIN_DISABLED;
            }
            break;
        case BACKPLANE_POWER_SHED_PENDING:
            if (port->channel_state != POWER_STATE_ACTIVE) {
                port->allocated_milliamps = 0U;
                port->management_state = !port->flags.module_detected ? BACKPLANE_POWER_ABSENT
                    : (port->channel_state == POWER_STATE_TRIPPED ? BACKPLANE_POWER_TRIPPED : BACKPLANE_POWER_WAITING);
                port->next_power_retry_ms = board->last_ms + BACKPLANE_POWER_RETRY_MS;
            }
            break;
        case BACKPLANE_POWER_ENABLE_PENDING:
            if (!port->flags.module_detected) {
                port->management_state = BACKPLANE_POWER_SHED_REQUESTED;
                port->next_power_retry_ms = board->last_ms;
            } else if (port->channel_state == POWER_STATE_ACTIVE) {
                port->management_state = BACKPLANE_POWER_POWERED;
            } else if (port->channel_state == POWER_STATE_TRIPPED) {
                port->allocated_milliamps = 0U;
                port->management_state = BACKPLANE_POWER_TRIPPED;
            }
            break;
        default:
            if (port->channel_state != POWER_STATE_ACTIVE) {
                port->allocated_milliamps = 0U;
            }
            if (!port->flags.module_detected) {
                port->management_state = port->channel_state == POWER_STATE_ACTIVE ? BACKPLANE_POWER_SHED_REQUESTED
                    : BACKPLANE_POWER_ABSENT;
                port->next_power_retry_ms = board->last_ms;
            } else if (port->channel_state == POWER_STATE_TRIPPED || port->management_state == BACKPLANE_POWER_TRIPPED) {
                port->management_state = BACKPLANE_POWER_TRIPPED;
            } else if (port->channel_state == POWER_STATE_ACTIVE) {
                port->management_state = BACKPLANE_POWER_POWERED;
            } else if (port->management_state != BACKPLANE_POWER_WAITING) {
                port->management_state = BACKPLANE_POWER_WAITING;
                port->next_power_retry_ms = board->last_ms;
            }
            break;
    }

    if (port->allocated_milliamps != previous_allocation) {
        for (size_t index = 0U; index < BACKPLANE_CHASSIS_ADDRESS; ++index) {
            for (size_t other_channel = 0U; other_channel < BACKPLANE_PORT_COUNT; ++other_channel) {
                Backplane_PortData *const waiting = &backplane->backplanes[index].ports[other_channel];
                if (waiting->management_state == BACKPLANE_POWER_WAITING) {
                    waiting->next_power_retry_ms = board->last_ms;
                }
            }
        }
    }
}

bool Backplane_QueueCommand(const BackplaneBus_Packet *packet, const size_t length) {
    if ((packet == NULL) || (length < BACKPLANE_BUS_SIZE_HEADER) ||
        (length > sizeof(*packet)) || (packet->header.address >= BACKPLANE_COUNT)) {
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
    if (packet->header.opcode == BACKPLANE_BUS_SET_ENABLED) {
        return Backplane_SetModuleEnabled(packet->header.address,
            packet->set_enabled.flags.port ? BACKPLANE_PORT_REAR : BACKPLANE_PORT_FRONT,
            packet->set_enabled.flags.enabled != 0U);
    }
    if (backplane->command_count >= BACKPLANE_COMMAND_QUEUE_SIZE) {
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
    uint32_t allocated_ma = 0U;
    uint32_t remaining_ma = 0U;
    for (size_t index = 0U; index < BACKPLANE_COUNT; ++index) {
        Backplane_StateData *const board = &backplane->backplanes[index];
        if (board->flags.awake &&
            (int32_t) (now_ms - (board->last_ms + BACKPLANE_STATUS_TIMEOUT_MS)) >= 0) {
            board->flags.awake = false;
        }
        for (size_t channel = 0U; channel < BACKPLANE_PORT_COUNT; ++channel) {
            const Backplane_PortData *const port = &board->ports[channel];
            allocated_ma += port->allocated_milliamps;
            switch (port->management_state) {
                case BACKPLANE_POWER_SHED_REQUESTED:
                case BACKPLANE_POWER_SHED_PENDING:
                case BACKPLANE_POWER_ADMIN_DISABLE_REQUESTED:
                case BACKPLANE_POWER_ADMIN_DISABLE_PENDING:
                    break;
                default:
                    remaining_ma += port->allocated_milliamps;
                    break;
            }
        }
    }

    /* Choose enough shutdowns to fit, but do not spend their capacity yet. */
    while (remaining_ma > backplane->power_budget_milliamps) {
        Backplane_PortData *largest = NULL;
        for (size_t index = 0U; index < BACKPLANE_CHASSIS_ADDRESS; ++index) {
            for (size_t channel = 0U; channel < BACKPLANE_PORT_COUNT; ++channel) {
                Backplane_PortData *const port = &backplane->backplanes[index].ports[channel];
                switch (port->management_state) {
                    case BACKPLANE_POWER_SHED_REQUESTED:
                    case BACKPLANE_POWER_SHED_PENDING:
                    case BACKPLANE_POWER_ADMIN_DISABLE_REQUESTED:
                    case BACKPLANE_POWER_ADMIN_DISABLE_PENDING:
                        continue;
                    default:
                        break;
                }
                if ((port->allocated_milliamps > 0U) &&
                    ((largest == NULL) || (port->allocated_milliamps > largest->allocated_milliamps))) {
                    largest = port;
                }
            }
        }
        if (largest == NULL) {
            break; /* Only protected chassis allocations remain. */
        }
        remaining_ma -= largest->allocated_milliamps;
        largest->management_state = BACKPLANE_POWER_SHED_REQUESTED;
        largest->next_power_retry_ms = now_ms;
    }

    for (size_t index = 0U; index < BACKPLANE_CHASSIS_ADDRESS; ++index) {
        Backplane_StateData *const board = &backplane->backplanes[index];
        if (!board->flags.awake) {
            continue;
        }
        for (size_t channel = 0U; channel < BACKPLANE_PORT_COUNT; ++channel) {
            Backplane_PortData *const port = &board->ports[channel];
            if ((port->management_state != BACKPLANE_POWER_WAITING) ||
                !port->flags.module_detected || port->channel_state == POWER_STATE_TRIPPED ||
                (int32_t) (now_ms - port->next_power_retry_ms) < 0) {
                continue;
            }
            if ((allocated_ma <= backplane->power_budget_milliamps) &&
                (backplane->power_budget_milliamps - allocated_ma >= BACKPLANE_STARTUP_ALLOCATION_MA)) {
                port->allocated_milliamps = BACKPLANE_STARTUP_ALLOCATION_MA;
                allocated_ma += BACKPLANE_STARTUP_ALLOCATION_MA;
                port->management_state = BACKPLANE_POWER_ENABLE_PENDING;
            } else {
                port->next_power_retry_ms = now_ms + BACKPLANE_POWER_RETRY_MS;
            }
        }
    }

    if ((int32_t) (now_ms - backplane->next_send_ms) < 0) {
        return;
    }

    bool sent = false;
    if ((int32_t) (now_ms - backplane->next_inquiry_ms) >= 0) {
        sent = backplane_send_inquiry();
        if (sent) {
            backplane->next_inquiry_ms = now_ms + BACKPLANE_BUSY_INQUIRY_INTERVAL_MS;
        }
    } else {
        /* Send new shutdowns largest first; oldest retries avoid starving ports. */
        Backplane_PortData *selected = NULL;
        uint8_t selected_address = 0U;
        size_t selected_channel = 0U;
        unsigned int selected_priority = 3U;
        for (size_t index = 0U; index < BACKPLANE_CHASSIS_ADDRESS; ++index) {
            Backplane_StateData *const board = &backplane->backplanes[index];
            if (!board->flags.awake) {
                continue;
            }
            for (size_t channel = 0U; channel < BACKPLANE_PORT_COUNT; ++channel) {
                Backplane_PortData *const port = &board->ports[channel];
                unsigned int priority;
                switch (port->management_state) {
                    case BACKPLANE_POWER_SHED_REQUESTED:
                    case BACKPLANE_POWER_ADMIN_DISABLE_REQUESTED:
                        priority = 0U;
                        break;
                    case BACKPLANE_POWER_SHED_PENDING:
                    case BACKPLANE_POWER_ADMIN_DISABLE_PENDING:
                        priority = 1U;
                        break;
                    case BACKPLANE_POWER_ENABLE_PENDING:
                        priority = 2U;
                        break;
                    default:
                        continue;
                }
                if ((int32_t) (now_ms - port->next_power_retry_ms) < 0) {
                    continue;
                }
                if ((selected == NULL) || (priority < selected_priority) ||
                    ((priority == selected_priority) &&
                     ((priority == 0U) ? port->allocated_milliamps > selected->allocated_milliamps
                      : (int32_t) (port->next_power_retry_ms - selected->next_power_retry_ms) < 0))) {
                    selected = port;
                    selected_address = (uint8_t) index;
                    selected_channel = channel;
                    selected_priority = priority;
                }
            }
        }
        if (selected != NULL) {
            BackplaneBus_Packet packet = {0};
            packet.header.address = selected_address;
            packet.header.opcode = BACKPLANE_BUS_SET_ENABLED;
            packet.set_enabled.flags.port = selected_channel == BACKPLANE_PORT_REAR;
            packet.set_enabled.flags.enabled = selected->management_state == BACKPLANE_POWER_ENABLE_PENDING;
            if (BackplaneBus_Send(&packet, SIZE_BACKPLANE_BUS_SET_ENABLED)) {
                selected->next_power_retry_ms = now_ms + BACKPLANE_POWER_RETRY_MS;
                if (selected->management_state == BACKPLANE_POWER_SHED_REQUESTED) {
                    selected->management_state = BACKPLANE_POWER_SHED_PENDING;
                } else if (selected->management_state == BACKPLANE_POWER_ADMIN_DISABLE_REQUESTED) {
                    selected->management_state = BACKPLANE_POWER_ADMIN_DISABLE_PENDING;
                }
                backplane->next_send_ms = now_ms + BACKPLANE_POLL_INTERVAL_MS;
            }
            return;
        }
        if (backplane->command_count > 0U) {
            sent = backplane_send_queued_command();
        } else {
            sent = backplane_send_inquiry();
            if (sent) {
                backplane->next_inquiry_ms = now_ms + BACKPLANE_BUSY_INQUIRY_INTERVAL_MS;
            }
        }
    }
    if (sent) {
        backplane->next_send_ms = now_ms + BACKPLANE_POLL_INTERVAL_MS;
    }
}
