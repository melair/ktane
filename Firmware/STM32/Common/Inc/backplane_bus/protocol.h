#ifndef BACKPLANE_BUS_PROTOCOL_H
#define BACKPLANE_BUS_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

/*
 * Add each packet here once. The list generates both BackplaneBus_OpCode and
 * the minimum wire size for each packet.
 */
#define BACKPLANE_BUS_PROTOCOL_PACKETS(X)             \
    X(BACKPLANE_BUS_INQUIRY, 0x00U, inquiry)          \
    X(BACKPLANE_BUS_STATUS,  0x01U, status)           \
    X(BACKPLANE_BUS_CONTROL, 0x02U, control)

#define BACKPLANE_BUS_PROTOCOL_DECLARE_OPCODE(opcode, value, member) opcode = value,

typedef enum : uint8_t {
    BACKPLANE_BUS_PROTOCOL_PACKETS(BACKPLANE_BUS_PROTOCOL_DECLARE_OPCODE)
} BackplaneBus_OpCode;

#undef BACKPLANE_BUS_PROTOCOL_DECLARE_OPCODE

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
            struct {
                unsigned _front_rear :1;

                unsigned disabled :1;
                unsigned output_enabled :1;
                unsigned tripped :1;
                unsigned module_detected :1;
            } flags;

            uint8_t current_limit_deciamps;
            uint16_t current_milliamps;
        } status;

        struct {
            struct {
                unsigned _front_rear :1;
                unsigned disable :1;
            } flags;

            uint8_t current_limit_deciamps;
        } control;
    };
} BackplaneBus_Packet;

#pragma pack(pop)

#define BACKPLANE_BUS_PROTOCOL_DECLARE_SIZE(opcode, value, member) \
    SIZE_##opcode = offsetof(BackplaneBus_Packet, member) + \
                    sizeof(((BackplaneBus_Packet *) 0)->member),

enum {
    BACKPLANE_BUS_SIZE_HEADER = sizeof(((BackplaneBus_Packet *) 0)->header),
    BACKPLANE_BUS_PROTOCOL_PACKETS(BACKPLANE_BUS_PROTOCOL_DECLARE_SIZE)
};

#undef BACKPLANE_BUS_PROTOCOL_DECLARE_SIZE

#endif // BACKPLANE_BUS_PROTOCOL_H
