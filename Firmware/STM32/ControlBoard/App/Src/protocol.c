#include <stdbool.h>
#include <stddef.h>
#include "mode.h"
#include "protocol.h"
#include "sys/can.h"
#include "sys/nvm.h"
#include "sys/rng.h"

typedef struct {
    bool id_negotiated;
    uint8_t id;
} protocol_t;

typedef struct {
    OpCode opcode;
    uint8_t minimumLength;

    void (*callback)(uint8_t id, OpCode opcode, Packet *packet);
} PacketRegistry;

static protocol_t protocol = {
    .id = 0U,
    .id_negotiated = false,
};

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

void Protocol_Init(void) {
    protocol.id = TRNG_Rand8Range(1U, 254U);
    protocol.id_negotiated = false;

    const NVM_Query query = {
        .type = UINT8,
        .id = MODE_CFG(MODE_NONE, MODE_CONFIG_CAN_ID),
        .data = &protocol.id,
    };

    NVM_Read(&query, 1U);
}

void Protocol_Service(void) {
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

    const PacketRegistry *definition = lookup_registry_entry(opcode);

    if (definition == NULL) {
        return;
    }

    packet->header.opcode = (uint8_t) ((uint16_t) opcode & OPCODE_MASK);

    const uint16_t mailbox = ((uint16_t) opcode & SUBSYS_MASK) | protocol.id;
    CAN_Queue(mailbox, definition->minimumLength, packet);
}
