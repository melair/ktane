#include <xc.h>
#include <stdint.h>
#include "protocol.h"
#include "can.h"
#include "packet.h"

#ifdef SUPPORT_PROTOCOL_MODULE
#include "../Application.X/module.h"
#include "../Application.X/opts.h"
#endif

#ifdef SUPPORT_PROTOCOL_GAME
#include "../Application.X/game.h"
#include "../Application.X/sound.h"
#endif

#ifdef SUPPORT_PROTOCOL_NETWORK
#ifdef SUPPORT_CAN_AUTO_ADDRESS
#include "../Application.X/can_auto_address.h"
#endif
#endif

#ifdef SUPPORT_PROTOCOL_FIRMWARE
#ifdef SUPPORT_FIRMWARE_SERVER
#include "../Application.X/fw_server.h"
#endif
#include "../common/fw_updater.h"
#endif

packet_t packet_outgoing;

/* Local function prototypes. */
void packet_route_module(uint8_t src, packet_t *packet);
void packet_route_game(uint8_t src, packet_t *packet);
void packet_route_network(uint8_t src, packet_t *packet);
void packet_route_firmware(uint8_t src, packet_t *packet);

void packet_send(uint8_t prefix, uint8_t opcode, uint8_t size, packet_t *packet) {
    packet->opcode = opcode;
    can_send(prefix, size, (uint8_t *) packet);
}

void packet_route(uint8_t src, uint8_t prefix, packet_t *packet) {
    /* Detect a CAN ID conflict, report as an error. This ignores the packet,
     * this might result in a desync of the game - but it's a critical error
     * and we can not continue. We have to allow conflicts in the NETWORK
     * prefix otherwise we can't use autoassignment. */
    if (prefix != PREFIX_NETWORK && src == can_get_id()) {
        // TODO: Raise can conflict error.
        // module_error_raise(MODULE_ERROR_CAN_ID_CONFLICT, true);
        return;
    }

    switch (prefix) {
#ifdef SUPPORT_PROTOCOL_MODULE
        case PREFIX_MODULE:
            packet_route_module(src, packet);
            break;
#endif
#ifdef SUPPORT_PROTOCOL_GAME
        case PREFIX_GAME:
            packet_route_game(src, packet);
            break;
#endif
#ifdef SUPPORT_PROTOCOL_NETWORK
        case PREFIX_NETWORK:
            packet_route_network(src, packet);
            break;
#endif
#ifdef SUPPORT_PROTOCOL_FIRMWARE
        case PREFIX_FIRMWARE:
            packet_route_firmware(src, packet);
            break;
#endif
        default:
            // TODO: Raise error for no packet handler.
            // module_error_raise(MODULE_ERROR_PROTOCOL_UNKNOWN | (PREFIX_FIRMWARE << 8) | payload[0], true);
            break;
    }
}

#ifdef SUPPORT_PROTOCOL_MODULE

void packet_route_module(uint8_t src, packet_t *packet) {
    switch (packet->opcode) {
        case OPCODE_MODULE_ANNOUNCEMENT:
            module_receive_announce(src, packet);
            break;

        case OPCODE_MODULE_RESET:
            module_receive_reset(src, packet);
            break;

        case OPCODE_MODULE_IDENTIFY:
            module_receive_identify(src, packet);
            break;

        case OPCODE_MODULE_MODE_SET:
            module_receive_mode_set(src, packet);
            break;

        case OPCODE_MODULE_SPECIAL_FUNCTION:
            module_receive_special_function(src, packet);
            break;

        case OPCODE_MODULE_GLOBAL_CONFIG:
            module_receive_global_config(src, packet);
            break;

        case OPCODE_MODULE_OPT_SET:
            opts_receive_opt_set(src, packet);
            break;

        case OPCODE_MODULE_ERROR:
            module_receive_error(src, packet);
            break;

        case OPCODE_MODULE_POWER_OFF:
            module_receive_power_off(src, packet);
            break;

        case OPCODE_MODULE_POWER_STATE:
            module_receive_power_state(src, packet);
            break;

        default:
            break;
    }
}
#endif

#ifdef SUPPORT_PROTOCOL_GAME

void packet_route_game(uint8_t src, packet_t *packet) {
    switch (packet->opcode) {
        case OPCODE_GAME_STATE:
            game_receive_update(src, packet);
            break;

        case OPCODE_GAME_MODULE_CONFIG:
            game_receive_module_config(src, packet);
            break;

        case OPCODE_GAME_MODULE_STATE:
            game_receive_module_update_state(src, packet);
            break;

        case OPCODE_GAME_MODULE_STRIKE:
            game_receive_strike_update(src, packet);
            break;

        case OPCODE_GAME_SOUND_PLAY:
            sound_receive_play(src, packet);
            break;

        case OPCODE_GAME_SOUND_STOP:
            sound_receive_stop(src, packet);
            break;

        default:
            break;
    }
}
#endif

#ifdef SUPPORT_PROTOCOL_NETWORK

void packet_route_network(uint8_t src, packet_t *packet) {
    switch (packet->opcode) {
#ifdef SUPPORT_CAN_AUTO_ADDRESS
        case OPCODE_NETWORK_ADDRESS_ANNOUNCE:
            can_address_receive_announce(src, packet);
            break;

        case OPCODE_NETWORK_ADDRESS_NAK:
            can_address_receive_nak(src, packet);
            break;
#endif
        default:
            break;
    }
}
#endif

#ifdef SUPPORT_PROTOCOL_FIRMWARE

void packet_route_firmware(uint8_t src, packet_t *packet) {
    switch (packet->opcode) {
#ifdef SUPPORT_FIRMWARE_SERVER
        case OPCODE_FIRMWARE_REQUEST:
            fw_server_recieve_header_request(src, packet);
            break;

        case OPCODE_FIRMWARE_PAGE_REQUEST:
            fw_server_recieve_page_request(src, packet);
            break;
#endif
        case OPCODE_FIRMWARE_HEADER:
            fw_updater_receive_header(src, packet);
            break;

        case OPCODE_FIRMWARE_PAGE_RESPONSE:
            fw_updater_receive_page(src, packet);
            break;

        default:
            break;
    }
}
#endif
