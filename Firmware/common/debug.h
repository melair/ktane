#ifndef DEBUG_H
#define	DEBUG_H

#include "../common/packet.h"

#define DEBUG_PACKET packet_outgoing.debug.announce.mode
#define DEBUG_SEND(mode, type) do {\
packet_outgoing.debug.announce.mode_type = mode;\
packet_outgoing.debug.announce.message_type = type;\
packet_send(PREFIX_DEBUG, OPCODE_DEBUG_ANNOUNCE, SIZE_DEBUG_ANNOUNCE, &packet_outgoing);\
} while(0)

#define DEBUG_WIRES_SOLUTION 0x00

#define DEBUG_KEYS_PLAN 0x00

#endif	/* DEBUG_H */

