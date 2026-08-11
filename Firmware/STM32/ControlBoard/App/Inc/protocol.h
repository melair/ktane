#ifndef PROTOCOL_H
#define PROTOCOL_H
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SUBSYS_GAME       0x000U
#define SUBSYS_MODULE     0x200U
#define SUBSYS_FIRMWARE   0x600U
#define SUBSYS_DEBUGGING  0x700U

#define SUBSYS_MASK 0x700U
#define OPCODE_MASK 0x0FFU
#define MAILBOX_ID_MASK 0x0FFU

/*
 * Add each packet here once. The list generates both OpCode and the
 * routing metadata consumed by Protocol_Receive(). Replace NULL with the
 * packet's consumer callback when it is implemented.
 */
#define PROTOCOL_PACKETS(X) \
    X(MODULE_ADDRESS_ANNOUNCE, SUBSYS_MODULE | 0x00U, module.address_announce, handle_address_announce) \
    X(MODULE_ADDRESS_NAK,      SUBSYS_MODULE | 0x01U, module.address_nak,      handle_address_nak) \
    X(MODULE_ANNOUNCE,         SUBSYS_MODULE | 0x10U, module.announce,         NULL)

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
                uint32_t serial;
            } address_announce;

            struct {
                uint32_t serial;
            } address_nak;

            struct {
                uint32_t serial;
                uint8_t mode;
                struct {
                    uint8_t chassis_location :4;
                    uint8_t reserved :4;
                } flags;
            } announce;
        } module;
    };
} Packet;

#pragma pack(pop)

#define PROTOCOL_DECLARE_SIZE(opcode, value, member, callback) \
    SIZE_##opcode = offsetof(Packet, member) + sizeof(((Packet *)0)->member),

enum {
    SIZE_HEADER = sizeof(((Packet *)0)->header),
    PROTOCOL_PACKETS(PROTOCOL_DECLARE_SIZE)
};

#undef PROTOCOL_DECLARE_SIZE

void Protocol_Init(void);

void Protocol_Service(void);

void Protocol_Receive(uint16_t mailbox, uint8_t length, void *data);

void Protocol_Send(OpCode opcode, Packet *packet);

#ifdef __cplusplus
}
#endif

#endif //PROTOCOL_H
