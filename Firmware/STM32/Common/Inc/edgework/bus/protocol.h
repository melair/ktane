#ifndef EDGEWORK_BUS_PROTOCOL_H
#define EDGEWORK_BUS_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

#include "edgework/data.h"

/*
 * Add each packet here once. The list generates both EdgeworkBus_OpCode and
 * the minimum wire size for each packet.
 */
#define EDGEWORK_BUS_PROTOCOL_PACKETS(X)                         \
    X(EDGEWORK_BUS_INQUIRY,         0x00U, inquiry)              \
    X(EDGEWORK_BUS_STATUS,          0x01U, status)               \
    X(EDGEWORK_BUS_DISPLAY,         0x02U, display)              \
    X(EDGEWORK_BUS_CLEAR,           0x03U, clear)                \
    X(EDGEWORK_BUS_SET_MODE,        0xf0U, set_mode)             \
    X(EDGEWORK_BUS_SET_SLOT_ADDRESS, 0xf1U, set_slot_address)

#define EDGEWORK_BUS_PROTOCOL_DECLARE_OPCODE(opcode, value, member) opcode = value,

typedef enum : uint8_t {
    EDGEWORK_BUS_PROTOCOL_PACKETS(EDGEWORK_BUS_PROTOCOL_DECLARE_OPCODE)
} EdgeworkBus_OpCode;

#undef EDGEWORK_BUS_PROTOCOL_DECLARE_OPCODE

#pragma pack(push, 1)

typedef struct {
    struct {
        uint8_t address;
        uint8_t opcode;

        struct {
            unsigned eor :1;
        } flags;
    } header;

    union {
        struct {
        } inquiry;

        struct {
            uint8_t mode;
            edgework_state_t data;
            uint8_t state;
        } status;

        struct {
            edgework_state_t data;
        } display;

        struct {
        } clear;

        struct {
            uint8_t new_mode;
        } set_mode;

        struct {
            uint8_t new_address;
        } set_slot_address;
    };
} EdgeworkBus_Packet;

#pragma pack(pop)

#define EDGEWORK_BUS_PROTOCOL_DECLARE_SIZE(opcode, value, member) \
    SIZE_##opcode = offsetof(EdgeworkBus_Packet, member) + \
                    sizeof(((EdgeworkBus_Packet *) 0)->member),

enum {
    EDGEWORK_BUS_SIZE_HEADER = sizeof(((EdgeworkBus_Packet *) 0)->header),
    EDGEWORK_BUS_PROTOCOL_PACKETS(EDGEWORK_BUS_PROTOCOL_DECLARE_SIZE)
};

#undef EDGEWORK_BUS_PROTOCOL_DECLARE_SIZE

#endif // EDGEWORK_BUS_PROTOCOL_H
