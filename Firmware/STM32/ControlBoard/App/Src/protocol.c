#include <stdbool.h>
#include <stddef.h>
#include "mode.h"
#include "nodes.h"
#include "protocol.h"
#include "sys/can.h"
#include "sys/fsm.h"
#include "sys/mcu_init.h"
#include "sys/nvm.h"
#include "sys/rng.h"
#include "stm32h5xx_hal.h"
#include "stm32h5xx_it.h"

#define PROTOCOL_IDENTIFIER_ANNOUNCEMENT_COUNT 3U
#define PROTOCOL_IDENTIFIER_WAIT_MIN_MS 50U
#define PROTOCOL_IDENTIFIER_WAIT_MAX_MS 100U
#define PROTOCOL_IDENTIFIER_FINAL_WAIT_MS 50U

typedef enum {
    PROTOCOL_FSM_STATE_WAIT_ANNOUNCE = 0,
    PROTOCOL_FSM_STATE_ANNOUNCE,
    PROTOCOL_FSM_STATE_WAIT_CONFIRM,
    PROTOCOL_FSM_STATE_RETRY,
    PROTOCOL_FSM_STATE_CONFIRMED,
} Protocol_FSM_State;

typedef struct {
    uint8_t identifier;

    struct {
        FSM fsm;
        uint32_t next_action_at;
        uint8_t stored_identifier;
        uint8_t announcement_count;
        bool identifier_negotiated;
        bool collision_detected;
    } identifier_negotiation;
} protocol_t;

typedef struct {
    OpCode opcode;
    uint8_t minimumLength;

    void (*callback)(Protocol_Message *message);
} PacketRegistry;

static void handle_identifier_announce(Protocol_Message *message);
static void handle_identifier_nak(Protocol_Message *message);

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

static void handle_identifier_announce(Protocol_Message *message) {
    if ((message->direction != CAN_DIRECTION_IN) ||
        (message->identifier != protocol.identifier)) {
        return;
    }

    Packet nak = {0};
    nak.node.identifier_nak.serial = UID;
    Protocol_Send(MODULE_IDENTIFIER_NAK, &nak);

    if (!protocol.identifier_negotiation.identifier_negotiated) {
        protocol.identifier_negotiation.collision_detected = true;
    }
}

static void handle_identifier_nak(Protocol_Message *message) {
    if (!protocol.identifier_negotiation.identifier_negotiated &&
        (message->direction == CAN_DIRECTION_IN) &&
        (message->identifier == protocol.identifier)) {
        protocol.identifier_negotiation.collision_detected = true;
    }
}

static void protocol_fsm_wait_announce_enter(FSM *fsm) {
    protocol.identifier_negotiation.next_action_at = HAL_GetTick() +
        TRNG_Rand8Range(PROTOCOL_IDENTIFIER_WAIT_MIN_MS,
                       PROTOCOL_IDENTIFIER_WAIT_MAX_MS);
}

static void protocol_fsm_wait_announce_service(FSM *fsm) {
    if (protocol.identifier_negotiation.collision_detected) {
        FSM_Transition(fsm, PROTOCOL_FSM_STATE_RETRY);
    } else if (action_is_due(protocol.identifier_negotiation.next_action_at)) {
        FSM_Transition(fsm, PROTOCOL_FSM_STATE_ANNOUNCE);
    }
}

static void protocol_fsm_announce_enter(FSM *fsm) {
    if (protocol.identifier_negotiation.collision_detected) {
        FSM_Transition(fsm, PROTOCOL_FSM_STATE_RETRY);
        return;
    }

    Packet announcement = {0};
    announcement.node.identifier_announce.serial = UID;
    Protocol_Send(MODULE_IDENTIFIER_ANNOUNCE, &announcement);

    protocol.identifier_negotiation.announcement_count++;
    if (protocol.identifier_negotiation.announcement_count < PROTOCOL_IDENTIFIER_ANNOUNCEMENT_COUNT) {
        FSM_Transition(fsm, PROTOCOL_FSM_STATE_WAIT_ANNOUNCE);
    } else {
        FSM_Transition(fsm, PROTOCOL_FSM_STATE_WAIT_CONFIRM);
    }
}

static void protocol_fsm_wait_confirm_enter(FSM *fsm) {
    protocol.identifier_negotiation.next_action_at = HAL_GetTick() + PROTOCOL_IDENTIFIER_FINAL_WAIT_MS;
}

static void protocol_fsm_wait_confirm_service(FSM *fsm) {
    if (protocol.identifier_negotiation.collision_detected) {
        FSM_Transition(fsm, PROTOCOL_FSM_STATE_RETRY);
    } else if (action_is_due(protocol.identifier_negotiation.next_action_at)) {
        FSM_Transition(fsm, PROTOCOL_FSM_STATE_CONFIRMED);
    }
}

static void protocol_fsm_retry_enter(FSM *fsm) {
    const uint8_t previous_identifier = protocol.identifier;

    do {
        protocol.identifier = TRNG_Rand8Range(1U, 254U);
    } while (protocol.identifier == previous_identifier);

    protocol.identifier_negotiation.identifier_negotiated = false;
    protocol.identifier_negotiation.collision_detected = false;
    protocol.identifier_negotiation.announcement_count = 0U;
    FSM_Transition(fsm, PROTOCOL_FSM_STATE_WAIT_ANNOUNCE);
}

static void protocol_fsm_confirmed_enter(FSM *fsm) {
    protocol.identifier_negotiation.identifier_negotiated = true;
    protocol.identifier_negotiation.collision_detected = false;

    if (protocol.identifier == protocol.identifier_negotiation.stored_identifier) {
        return;
    }

    const NVM_Query query = {
        .type = UINT8,
        .id = MODE_CFG(MODE_NONE, MODE_CONFIG_CAN_IDENTIFIER),
        .data = &protocol.identifier,
    };

    if (NVM_Write(&query)) {
        protocol.identifier_negotiation.stored_identifier = protocol.identifier;
    }
}

void Protocol_Init(void) {
    protocol = (protocol_t) {
        .identifier = TRNG_Rand8Range(1U, 254U),
    };

    const NVM_Query query = {
        .type = UINT8,
        .id = MODE_CFG(MODE_NONE, MODE_CONFIG_CAN_IDENTIFIER),
        .data = &protocol.identifier_negotiation.stored_identifier,
    };

    NVM_Read(&query, 1U);

    if ((protocol.identifier_negotiation.stored_identifier >= 1U) &&
        (protocol.identifier_negotiation.stored_identifier <= 254U)) {
        protocol.identifier = protocol.identifier_negotiation.stored_identifier;
    } else {
        protocol.identifier_negotiation.stored_identifier = 0U;
    }

    if (!FSM_Init(&protocol.identifier_negotiation.fsm, protocol_fsm_states,
                  PROTOCOL_FSM_STATE_WAIT_ANNOUNCE, NULL)) {
        Error_Handler();
    }
}

void Protocol_Service(void) {
    /* Service the identifier negotiation fsm if we haven't agreed an address. */
    if (!protocol.identifier_negotiation.identifier_negotiated) {
        FSM_Service(&protocol.identifier_negotiation.fsm);
    }
}

void Protocol_Receive(const CAN_Packet *canPacket) {
    if ((canPacket == NULL) ||
        (canPacket->data == NULL) ||
        (canPacket->length < SIZE_HEADER)) {
        return;
    }

    const uint8_t module_identifier = canPacket->identifier & MODULE_IDENTIFIER_MASK;
    Packet *packet = (Packet *) canPacket->data;
    const OpCode opcode = (OpCode) ((canPacket->identifier & SUBSYS_MASK) | packet->header.opcode);

    const PacketRegistry *definition = lookup_registry_entry(opcode);

    if ((definition == NULL) || (canPacket->length < definition->minimumLength)) {
        return;
    }

    if (definition->callback != NULL) {
        Protocol_Message message = {
            .identifier = module_identifier,
            .opcode = opcode,
            .packet = packet,
            .direction = canPacket->direction,
            .timing = canPacket->timing,
        };
        definition->callback(&message);
    }
}

void Protocol_Send(const OpCode opcode, Packet *packet) {
    if (packet == NULL) {
        return;
    }

    if (!protocol.identifier_negotiation.identifier_negotiated &&
        (opcode != MODULE_IDENTIFIER_ANNOUNCE) &&
        (opcode != MODULE_IDENTIFIER_NAK)) {
        return;
    }

    const PacketRegistry *definition = lookup_registry_entry(opcode);

    if (definition == NULL) {
        return;
    }

    packet->header.opcode = (uint8_t) ((uint16_t) opcode & OPCODE_MASK);

    const uint16_t identifier = ((uint16_t) opcode & SUBSYS_MASK) | protocol.identifier;
    CAN_Queue(identifier, definition->minimumLength, packet);
}
