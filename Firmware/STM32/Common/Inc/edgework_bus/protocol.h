#ifndef EDGEWORK_BUS_PROTOCOL_H
#define EDGEWORK_BUS_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

/*
 * Add each packet here once. The list generates both EdgeworkBus_OpCode and
 * the minimum wire size for each packet.
 */
#define EDGEWORK_BUS_PROTOCOL_PACKETS(X)                         \
    X(EDGEWORK_BUS_INQUIRY,         0x00U, inquiry)              \
    X(EDGEWORK_BUS_STATUS,          0x01U, status)               \
    X(EDGEWORK_BUS_CHANGE,          0x02U, change)               \
    X(EDGEWORK_BUS_IDENTIFY,        0x03U, identify)             \
    X(EDGEWORK_BUS_SET_MODE,        0xf0U, set_mode)             \
    X(EDGEWORK_BUS_SET_SLOT_ADDRESS, 0xf1U, set_slot_address)

#define EDGEWORK_BUS_PROTOCOL_DECLARE_OPCODE(opcode, value, member) opcode = value,

typedef enum : uint8_t {
    EDGEWORK_BUS_PROTOCOL_PACKETS(EDGEWORK_BUS_PROTOCOL_DECLARE_OPCODE)
} EdgeworkBus_OpCode;

#undef EDGEWORK_BUS_PROTOCOL_DECLARE_OPCODE

#pragma pack(push, 1)

typedef union {
    struct {
        uint8_t value[6];
    } serial;

    struct {
        uint8_t indicator;

        struct {
            unsigned lit :1;
        } flags;
    } indicator;

    struct {
        unsigned parallel :1;
        unsigned serial :1;
        unsigned dvi :1;
        unsigned ps2 :1;
        unsigned rj45 :1;
        unsigned stereo_rca :1;
    } ports;

    struct {
        uint8_t value[6];

        struct {
            unsigned t1 :1;
            unsigned t2 :1;
            unsigned t3 :1;
            unsigned t4 :1;

            unsigned col1 :1;
            unsigned dot :1;
        } icons;

        struct {
            unsigned require_button :1;
            unsigned active_display :1;
        } flags;
    } twofa;
} edgework_data_t;

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
            edgework_data_t data;

            struct {
                unsigned active :1;
                unsigned ready :1;
                unsigned identifying :1;
            } flags;
        } status;

        struct {
            uint8_t active;
            edgework_data_t data;
        } change;

        struct {
            unsigned active :1;
        } identify;

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
