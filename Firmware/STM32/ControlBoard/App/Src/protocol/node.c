#include "protocol/node.h"

#include "protocol.h"

void Protocol_NodeIdentifierAnnounce(uint32_t serial) {
    Packet packet = {0};
    packet.node.identifier_announce.serial = serial;
    Protocol_Send(MODULE_IDENTIFIER_ANNOUNCE, &packet);
}

void Protocol_NodeIdentifierNak(uint32_t serial) {
    Packet packet = {0};
    packet.node.identifier_nak.serial = serial;
    Protocol_Send(MODULE_IDENTIFIER_NAK, &packet);
}

void Protocol_NodeAnnounce(uint32_t serial, uint32_t uptime, uint8_t mode,
                           uint8_t state, uint8_t chassis_location) {
    Packet packet = {0};
    packet.node.announce.serial = serial;
    packet.node.announce.uptime = uptime;
    packet.node.announce.mode = mode;
    packet.node.announce.state = state;
    packet.node.announce.flags.chassis_location = chassis_location;
    Protocol_Send(MODULE_ANNOUNCE, &packet);
}
