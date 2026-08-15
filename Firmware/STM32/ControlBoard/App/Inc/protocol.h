#ifndef PROTOCOL_H
#define PROTOCOL_H
#include <stddef.h>
#include <stdint.h>
#include "sys/can.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SUBSYS_GAME       0x000U
#define SUBSYS_NODE       0x200U
#define SUBSYS_FIRMWARE   0x600U
#define SUBSYS_DEBUGGING  0x700U

#define SUBSYS_MASK 0x700U
#define OPCODE_MASK 0x0FFU
#define MODULE_IDENTIFIER_MASK 0x0FFU

/*
 * Add each packet here once. The list generates both OpCode and the
 * routing metadata consumed by Protocol_Receive(). Replace NULL with the
 * packet's consumer callback when it is implemented.
 */
#define PROTOCOL_PACKETS(X) \
    X(MODULE_IDENTIFIER_ANNOUNCE, SUBSYS_NODE | 0x00U, node.identifier_announce, handle_identifier_announce) \
    X(MODULE_IDENTIFIER_NAK,      SUBSYS_NODE | 0x01U, node.identifier_nak,      handle_identifier_nak) \
    X(MODULE_ANNOUNCE,            SUBSYS_NODE | 0x10U, node.announce,            Nodes_ReceiveAnnounce)

#define PROTOCOL_DECLARE_OPCODE(opcode, value, member, callback) opcode = value,

typedef enum : uint16_t {
    PROTOCOL_PACKETS(PROTOCOL_DECLARE_OPCODE)
} OpCode;

#undef PROTOCOL_DECLARE_OPCODE

#pragma pack(push, 1)

typedef struct {
    struct {
        uint8_t opcode;
    } header;

    union {
        union {
            struct {
                uint8_t state;
            } request_transition;

            struct {
                uint32_t seed;

                uint8_t mode;
                uint8_t state;
                uint8_t strikes;

                struct {
                    uint32_t time_in_us;
                } clock;

                struct {
                    uint16_t active;
                    uint16_t puzzle;
                    uint16_t needy;
                    uint16_t solved;
                } node_masks;
            } update;

            struct {
                uint8_t event;
                uint32_t timestamp;
            } mode_event;
        } game;

        union {
            struct {
                uint32_t serial;
            } identifier_announce;

            struct {
                uint32_t serial;
            } identifier_nak;

            struct {
                uint32_t serial;
                uint32_t uptime;
                uint8_t mode;
                uint8_t state;
                struct {
                    uint8_t chassis_location :4;
                    uint8_t reserved :4;
                } flags;
            } announce;
        } node;
    };
} Packet;

#pragma pack(pop)

typedef struct {
    uint8_t identifier;
    OpCode opcode;
    Packet *packet;

    CAN_Direction direction;
    CAN_Timing timing;
} Protocol_Message;

#define PROTOCOL_DECLARE_SIZE(opcode, value, member, callback) \
    SIZE_##opcode = offsetof(Packet, member) + sizeof(((Packet *)0)->member),

enum {
    SIZE_HEADER = sizeof(((Packet *)0)->header),
    PROTOCOL_PACKETS(PROTOCOL_DECLARE_SIZE)
};

#undef PROTOCOL_DECLARE_SIZE

void Protocol_Init(void);

void Protocol_Service(void);

void Protocol_Receive(const CAN_Packet *canPacket);

void Protocol_Send(OpCode opcode, Packet *packet);

#ifdef __cplusplus
}
#endif

#endif //PROTOCOL_H
