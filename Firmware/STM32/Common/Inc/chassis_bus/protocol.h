#ifndef CHASSIS_BUS_PROTOCOL_H
#define CHASSIS_BUS_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

/*
 * Add each packet here once. The list generates both ChassisBus_OpCode and
 * the minimum wire size for each packet.
 */
#define CHASSIS_BUS_PROTOCOL_PACKETS(X)             \
    X(CHASSIS_BUS_INQUIRY, 0x00U, inquiry)          \
    X(CHASSIS_BUS_STATUS,  0x01U, status)           \
    X(CHASSIS_BUS_CONTROL, 0x02U, control)

#define CHASSIS_BUS_PROTOCOL_DECLARE_OPCODE(opcode, value, member) opcode = value,

typedef enum : uint8_t {
    CHASSIS_BUS_PROTOCOL_PACKETS(CHASSIS_BUS_PROTOCOL_DECLARE_OPCODE)
} ChassisBus_OpCode;

#undef CHASSIS_BUS_PROTOCOL_DECLARE_OPCODE

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
                unsigned disable :1;
            } flags;

            uint8_t current_limit_deciamps;
        } control;
    };
} ChassisBus_Packet;

#pragma pack(pop)

#define CHASSIS_BUS_PROTOCOL_DECLARE_SIZE(opcode, value, member) \
    SIZE_##opcode = offsetof(ChassisBus_Packet, member) + \
                    sizeof(((ChassisBus_Packet *) 0)->member),

enum {
    CHASSIS_BUS_SIZE_HEADER = sizeof(((ChassisBus_Packet *) 0)->header),
    CHASSIS_BUS_PROTOCOL_PACKETS(CHASSIS_BUS_PROTOCOL_DECLARE_SIZE)
};

#undef CHASSIS_BUS_PROTOCOL_DECLARE_SIZE

#endif // CHASSIS_BUS_PROTOCOL_H
