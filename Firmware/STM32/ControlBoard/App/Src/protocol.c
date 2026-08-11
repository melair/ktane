#include <stdbool.h>
#include <stddef.h>
#include "mode.h"
#include "protocol.h"
#include "sys/can.h"
#include "sys/fsm.h"
#include "sys/mcu_init.h"
#include "sys/nvm.h"
#include "sys/rng.h"
#include "stm32h5xx_hal.h"
#include "stm32h5xx_it.h"

#define PROTOCOL_ADDRESS_ANNOUNCEMENT_COUNT 3U
#define PROTOCOL_ADDRESS_WAIT_MIN_MS 50U
#define PROTOCOL_ADDRESS_WAIT_MAX_MS 100U
#define PROTOCOL_ADDRESS_FINAL_WAIT_MS 50U

typedef enum {
    PROTOCOL_FSM_STATE_WAIT_ANNOUNCE = 0,
    PROTOCOL_FSM_STATE_ANNOUNCE,
    PROTOCOL_FSM_STATE_WAIT_CONFIRM,
    PROTOCOL_FSM_STATE_RETRY,
    PROTOCOL_FSM_STATE_CONFIRMED,
} Protocol_FSM_State;

typedef struct {
    uint8_t id;

    struct {
        FSM fsm;
        uint32_t next_action_at;
        uint8_t stored_id;
        uint8_t announcement_count;
        bool id_negotiated;
        bool collision_detected;
    } addressing;
} protocol_t;

typedef struct {
    OpCode opcode;
    uint8_t minimumLength;

    void (*callback)(uint8_t id, OpCode opcode, Packet *packet);
} PacketRegistry;

static void handle_address_announce(uint8_t id, OpCode opcode, Packet *packet);
static void handle_address_nak(uint8_t id, OpCode opcode, Packet *packet);

static void protocol_fsm_wait_announce_enter(FSM *fsm);
static void protocol_fsm_wait_announce_service(FSM *fsm);
static void protocol_fsm_announce_enter(FSM *fsm);
static void protocol_fsm_wait_confirm_enter(FSM *fsm);
static void protocol_fsm_wait_confirm_service(FSM *fsm);
static void protocol_fsm_retry_enter(FSM *fsm);
static void protocol_fsm_confirmed_enter(FSM *fsm);

static const FSM_State protocol_fsm_states[] = {
    [PROTOCOL_FSM_STATE_WAIT_ANNOUNCE] = {
        .enter = protocol_fsm_wait_announce_enter,
        .service = protocol_fsm_wait_announce_service,
        .next_mask = FSM_NEXT(PROTOCOL_FSM_STATE_ANNOUNCE) |
                     FSM_NEXT(PROTOCOL_FSM_STATE_RETRY),
    },
    [PROTOCOL_FSM_STATE_ANNOUNCE] = {
        .enter = protocol_fsm_announce_enter,
        .next_mask = FSM_NEXT(PROTOCOL_FSM_STATE_WAIT_ANNOUNCE) |
                     FSM_NEXT(PROTOCOL_FSM_STATE_WAIT_CONFIRM) |
                     FSM_NEXT(PROTOCOL_FSM_STATE_RETRY),
    },
    [PROTOCOL_FSM_STATE_WAIT_CONFIRM] = {
        .enter = protocol_fsm_wait_confirm_enter,
        .service = protocol_fsm_wait_confirm_service,
        .next_mask = FSM_NEXT(PROTOCOL_FSM_STATE_RETRY) |
                     FSM_NEXT(PROTOCOL_FSM_STATE_CONFIRMED),
    },
    [PROTOCOL_FSM_STATE_RETRY] = {
        .enter = protocol_fsm_retry_enter,
        .next_mask = FSM_NEXT(PROTOCOL_FSM_STATE_WAIT_ANNOUNCE),
    },
    [PROTOCOL_FSM_STATE_CONFIRMED] = {
        .enter = protocol_fsm_confirmed_enter,
    },
};

static protocol_t protocol = {0};

#define PROTOCOL_PACKET_DEFINITION(opcode, value, member, callback_) \
    {(opcode), (uint8_t)SIZE_##opcode, (callback_)},

static const PacketRegistry packetRegistry[] = {
    PROTOCOL_PACKETS(PROTOCOL_PACKET_DEFINITION)
};

#undef PROTOCOL_PACKET_DEFINITION

static const PacketRegistry *lookup_registry_entry(OpCode opcode) {
    for (size_t index = 0U; index < (sizeof(packetRegistry) / sizeof(packetRegistry[0])); index++) {
        const PacketRegistry *definition = &packetRegistry[index];

        if (definition->opcode == opcode) {
            return definition;
        }
    }

    return NULL;
}

static bool action_is_due(const uint32_t action_at) {
    return (int32_t)(HAL_GetTick() - action_at) >= 0;
}

static void handle_address_announce(const uint8_t id, const OpCode opcode, Packet *packet) {
    if (id != protocol.id) {
        return;
    }

    Packet nak = {0};
    nak.module.address_nak.serial = UID;
    Protocol_Send(MODULE_ADDRESS_NAK, &nak);

    if (!protocol.addressing.id_negotiated) {
        protocol.addressing.collision_detected = true;
    }
}

static void handle_address_nak(const uint8_t id, const OpCode opcode, Packet *packet) {
    if (!protocol.addressing.id_negotiated && (id == protocol.id)) {
        protocol.addressing.collision_detected = true;
    }
}

static void protocol_fsm_wait_announce_enter(FSM *fsm) {
    protocol.addressing.next_action_at = HAL_GetTick() +
                                         TRNG_Rand8Range(PROTOCOL_ADDRESS_WAIT_MIN_MS,
                                                         PROTOCOL_ADDRESS_WAIT_MAX_MS);
}

static void protocol_fsm_wait_announce_service(FSM *fsm) {
    if (protocol.addressing.collision_detected) {
        FSM_Transition(fsm, PROTOCOL_FSM_STATE_RETRY);
    } else if (action_is_due(protocol.addressing.next_action_at)) {
        FSM_Transition(fsm, PROTOCOL_FSM_STATE_ANNOUNCE);
    }
}

static void protocol_fsm_announce_enter(FSM *fsm) {
    if (protocol.addressing.collision_detected) {
        FSM_Transition(fsm, PROTOCOL_FSM_STATE_RETRY);
        return;
    }

    Packet announcement = {0};
    announcement.module.address_announce.serial = UID;
    Protocol_Send(MODULE_ADDRESS_ANNOUNCE, &announcement);

    protocol.addressing.announcement_count++;
    if (protocol.addressing.announcement_count < PROTOCOL_ADDRESS_ANNOUNCEMENT_COUNT) {
        FSM_Transition(fsm, PROTOCOL_FSM_STATE_WAIT_ANNOUNCE);
    } else {
        FSM_Transition(fsm, PROTOCOL_FSM_STATE_WAIT_CONFIRM);
    }
}

static void protocol_fsm_wait_confirm_enter(FSM *fsm) {
    protocol.addressing.next_action_at = HAL_GetTick() + PROTOCOL_ADDRESS_FINAL_WAIT_MS;
}

static void protocol_fsm_wait_confirm_service(FSM *fsm) {
    if (protocol.addressing.collision_detected) {
        FSM_Transition(fsm, PROTOCOL_FSM_STATE_RETRY);
    } else if (action_is_due(protocol.addressing.next_action_at)) {
        FSM_Transition(fsm, PROTOCOL_FSM_STATE_CONFIRMED);
    }
}

static void protocol_fsm_retry_enter(FSM *fsm) {
    const uint8_t previous_id = protocol.id;

    do {
        protocol.id = TRNG_Rand8Range(1U, 254U);
    } while (protocol.id == previous_id);

    protocol.addressing.id_negotiated = false;
    protocol.addressing.collision_detected = false;
    protocol.addressing.announcement_count = 0U;
    FSM_Transition(fsm, PROTOCOL_FSM_STATE_WAIT_ANNOUNCE);
}

static void protocol_fsm_confirmed_enter(FSM *fsm) {
    protocol.addressing.id_negotiated = true;
    protocol.addressing.collision_detected = false;

    if (protocol.id == protocol.addressing.stored_id) {
        return;
    }

    const NVM_Query query = {
        .type = UINT8,
        .id = MODE_CFG(MODE_NONE, MODE_CONFIG_CAN_ID),
        .data = &protocol.id,
    };

    if (NVM_Write(&query)) {
        protocol.addressing.stored_id = protocol.id;
    }
}

void Protocol_Init(void) {
    protocol = (protocol_t) {
        .id = TRNG_Rand8Range(1U, 254U),
    };

    const NVM_Query query = {
        .type = UINT8,
        .id = MODE_CFG(MODE_NONE, MODE_CONFIG_CAN_ID),
        .data = &protocol.addressing.stored_id,
    };

    NVM_Read(&query, 1U);

    if ((protocol.addressing.stored_id >= 1U) && (protocol.addressing.stored_id <= 254U)) {
        protocol.id = protocol.addressing.stored_id;
    } else {
        protocol.addressing.stored_id = 0U;
    }

    if (!FSM_Init(&protocol.addressing.fsm, protocol_fsm_states,
                  PROTOCOL_FSM_STATE_WAIT_ANNOUNCE, NULL)) {
        Error_Handler();
    }
}

void Protocol_Service(void) {
    if (!protocol.addressing.id_negotiated) {
        FSM_Service(&protocol.addressing.fsm);
    }
}

void Protocol_Receive(uint16_t mailbox, uint8_t length, void *data) {
    if ((data == NULL) || (length < SIZE_HEADER)) {
        return;
    }

    uint8_t id = mailbox & MAILBOX_ID_MASK;
    Packet *packet = (Packet *) data;
    OpCode opcode = (OpCode) ((mailbox & SUBSYS_MASK) | packet->header.opcode);

    const PacketRegistry *definition = lookup_registry_entry(opcode);

    if ((definition == NULL) || (length < definition->minimumLength)) {
        return;
    }

    if (definition->callback != NULL) {
        definition->callback(id, opcode, packet);
    }
}

void Protocol_Send(OpCode opcode, Packet *packet) {
    if (packet == NULL) {
        return;
    }

    if (!protocol.addressing.id_negotiated &&
        (opcode != MODULE_ADDRESS_ANNOUNCE) &&
        (opcode != MODULE_ADDRESS_NAK)) {
        return;
    }

    const PacketRegistry *definition = lookup_registry_entry(opcode);

    if (definition == NULL) {
        return;
    }

    packet->header.opcode = (uint8_t) ((uint16_t) opcode & OPCODE_MASK);

    const uint16_t mailbox = ((uint16_t) opcode & SUBSYS_MASK) | protocol.id;
    CAN_Queue(mailbox, definition->minimumLength, packet);
}
