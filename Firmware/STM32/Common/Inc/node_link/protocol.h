#ifndef NODE_LINK_PROTOCOL_H
#define NODE_LINK_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

/*
 * Add each packet here once. The list generates both NodeLink_OpCode and the
 * minimum wire size for each packet.
 */
#define NODE_LINK_PROTOCOL_PACKETS(X) \
    X(NODE_LINK_ANNOUNCEMENT, 0x00U, announcement)

#define NODE_CHASSIS_LOCATION_SLOT_0  0x00U
#define NODE_CHASSIS_LOCATION_SLOT_1  0x01U
#define NODE_CHASSIS_LOCATION_SLOT_2  0x02U
#define NODE_CHASSIS_LOCATION_SLOT_3  0x03U
#define NODE_CHASSIS_LOCATION_SLOT_4  0x04U
#define NODE_CHASSIS_LOCATION_SLOT_5  0x05U
#define NODE_CHASSIS_LOCATION_SLOT_6  0x06U
#define NODE_CHASSIS_LOCATION_SLOT_7  0x07U
#define NODE_CHASSIS_LOCATION_SLOT_8  0x08U
#define NODE_CHASSIS_LOCATION_SLOT_9  0x09U
#define NODE_CHASSIS_LOCATION_SLOT_10 0x0aU
#define NODE_CHASSIS_LOCATION_SLOT_11 0x0bU
#define NODE_CHASSIS_LOCATION_CHASSIS 0x0eU
#define NODE_CHASSIS_LOCATION_UNKNOWN 0x0fU

#define NODE_LINK_PROTOCOL_DECLARE_OPCODE(opcode, value, member) opcode = value,

typedef enum : uint8_t {
    NODE_LINK_PROTOCOL_PACKETS(NODE_LINK_PROTOCOL_DECLARE_OPCODE)
} NodeLink_OpCode;

#undef NODE_LINK_PROTOCOL_DECLARE_OPCODE

#pragma pack(push, 1)

typedef struct {
    struct {
        uint8_t opcode;
    } header;

    union {
        struct {
            uint8_t chassis_location;
            uint8_t current_limit_deciamps;
        } announcement;
    };
} NodeLink_Packet;

#pragma pack(pop)

#define NODE_LINK_PROTOCOL_DECLARE_SIZE(opcode, value, member) \
    SIZE_##opcode = offsetof(NodeLink_Packet, member) + \
                    sizeof(((NodeLink_Packet *) 0)->member),

enum {
    NODE_LINK_SIZE_HEADER = sizeof(((NodeLink_Packet *) 0)->header),
    NODE_LINK_PROTOCOL_PACKETS(NODE_LINK_PROTOCOL_DECLARE_SIZE)
};

#undef NODE_LINK_PROTOCOL_DECLARE_SIZE

#endif // NODE_LINK_PROTOCOL_H
