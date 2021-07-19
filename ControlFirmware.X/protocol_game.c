#include <xc.h>
#include <stdint.h>
#include "can.h"
#include "module.h"
#include "game.h"
#include "protocol.h"
#include "protocol_game.h"

/* Local function prototypes. */
void protocol_game_state_receive(uint8_t id, uint8_t size, uint8_t *payload);
void protocol_game_module_config_recieve(uint8_t id, uint8_t size, uint8_t *payload);
void protocol_game_module_state_recieve(uint8_t id, uint8_t size, uint8_t *payload);
void protocol_game_module_strike_recieve(uint8_t id, uint8_t size, uint8_t *payload);

#define OPCODE_GAME_STATE           0x00

#define OPCODE_GAME_MODULE_CONFIG   0x10
#define OPCODE_GAME_MODULE_STATE    0x11
#define OPCODE_GAME_MODULE_STRIKE   0x12

/**
 * Handle reception of a new packet from CAN that is for the game management
 * prefix.
 * 
 * @param id CAN id for the source module, from the 8 least significant bits of the raw 11-bit CAN id
 * @param size size of CAN payload received
 * @param payload pointer to payload
 */
void protocol_game_receive(uint8_t id, uint8_t size, uint8_t *payload) {
    /* Safety check, if size is 0 we can not check the opcode. */
    if (size == 0) {
        return;
    }
    
    /* Switch to the correct packet based on opcode in packet. */
    switch (payload[0]) {
        case OPCODE_GAME_STATE:
            protocol_game_state_receive(id, size, payload);
            break;
        case OPCODE_GAME_MODULE_CONFIG:
            protocol_game_module_config_recieve(id, size, payload);
            break;            
        case OPCODE_GAME_MODULE_STATE:
            protocol_game_module_state_recieve(id, size, payload);
            break;            
        case OPCODE_GAME_MODULE_STRIKE:
            protocol_game_module_strike_recieve(id, size, payload);
            break;            
        default:
            /* Alert an unknown opcode has been received. */
            module_error_raise(MODULE_ERROR_PROTOCOL_UNKNOWN | (PREFIX_GAME << 8) | payload[0], true);
            break;
    }
}

/*
 * Firmware - Game State - (0x00)
 * 
 * Packet Layout:
 * 
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  State        |  Game Seed                                    |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Game Seed    |  Strikes      |  Minutes      | Seconds       |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Centiseconds |  Time Ratio   |                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  */

/**
 * Send a game state packet.
 */
void protocol_game_state_send(uint8_t state, uint32_t seed, uint8_t strikes_current, uint8_t strikes_total, uint8_t minutes, uint8_t seconds, uint8_t centiseconds, uint8_t time_ratio) {
    uint8_t payload[12];
    
    payload[0] = OPCODE_GAME_STATE;
    payload[1] = state;
    payload[2] = (seed >> 24) & 0xff;
    payload[3] = (seed >> 16) & 0xff;
    payload[4] = (seed >> 8) & 0xff;
    payload[5] = seed & 0xff;
    payload[6] = strikes_current;
    payload[7] = strikes_total;
    payload[8] = minutes;
    payload[9] = seconds;
    payload[10] = centiseconds;
    payload[11] = time_ratio;

    can_send(PREFIX_GAME, sizeof(payload), &payload[0]);
}

/**
 * Receive a game state packet, used to synchronise modules.
 * 
 * @param id CAN id for the source module, from the 8 least significant bits of the raw 11-bit CAN id
 * @param size size of CAN payload received
 * @param payload pointer to payload
 */
void protocol_game_state_receive(uint8_t id, uint8_t size, uint8_t *payload) {    
    /* Safety check. */
    if (size < 12) {
        return;
    }   
    
    uint8_t state = payload[1];
    uint8_t strikes_current = payload[6];
    uint8_t strikes_total = payload[7];
    uint8_t minutes = payload[8];
    uint8_t seconds = payload[9];
    uint8_t centiseconds = payload[10];
    uint8_t time_ratio = payload[11];
    
    uint8_t a = payload[2];
    uint8_t b = payload[3];
    uint8_t c = payload[4];
    uint8_t d = payload[5];
    
    uint32_t seed = ((uint32_t) a << 24) | ((uint32_t)b << 16) | ((uint32_t) c << 8) | ((uint32_t) d);
    
    game_update(state, seed, strikes_current, strikes_total, minutes, seconds, centiseconds, time_ratio);
}

/*
 * Firmware - Module Config - (0x10)
 * 
 * Packet Layout:
 * 
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |               |E| | | | | | | |               |               |
   |  Target ID    |N| | | | | | | |  Difficulty   |               | 
   |               |A| | | | | | | |               |               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  */

/**
 * Send a game state packet.
 */
void protocol_game_module_config_send(uint8_t target_id, bool enabled, uint8_t difficulty) {
    uint8_t payload[4];
    
    payload[0] = OPCODE_GAME_MODULE_CONFIG;    
    payload[1] = target_id;
    payload[2] = 0x00;
    
    if (enabled) {
        payload[2] |= 0b10000000;
    }
    
    payload[3] = difficulty;

    can_send(PREFIX_GAME, sizeof(payload), &payload[0]);
}

/**
 * Receive a game module config packet, used to set up modules during SETUP phase.
 * 
 * @param id CAN id for the source module, from the 8 least significant bits of the raw 11-bit CAN id
 * @param size size of CAN payload received
 * @param payload pointer to payload
 */
void protocol_game_module_config_recieve(uint8_t id, uint8_t size, uint8_t *payload) {    
    /* Safety check. */
    if (size < 4) {
        return;
    }              
        
    uint8_t target_id = payload[1];
    bool enabled = (payload[2] & 0b10000000) != 0;
    uint8_t difficulty = payload[3];
    
    game_module_config(target_id, enabled, difficulty);
}

/*
 * Firmware - Module State - (0x11)
 * 
 * Packet Layout:
 * 
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |R|S| | | | | | |                                               |
   |D|O| | | | | | |                                               |
   |Y|L| | | | | | |                                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  */

/**
 * Send a module state packet.
 */
void protocol_game_module_state_send(bool ready, bool solved) {
    uint8_t payload[2];
    
    payload[0] = OPCODE_GAME_MODULE_STATE;
    payload[1] = 0x00;
    
    if (ready) {
        payload[1] |= 0b10000000;
    }

    if (solved) {
        payload[1] |= 0b01000000;
    }
    
    can_send(PREFIX_GAME, sizeof(payload), &payload[0]);
}

/**
 * Receive a game module state packet, used to indicate when modules are ready
 * and solved.
 * 
 * @param id CAN id for the source module, from the 8 least significant bits of the raw 11-bit CAN id
 * @param size size of CAN payload received
 * @param payload pointer to payload
 */
void protocol_game_module_state_recieve(uint8_t id, uint8_t size, uint8_t *payload) {    
    /* Safety check. */
    if (size < 2) {
        return;
    }              
    
    bool ready = (payload[1] & 0b10000000) != 0;
    bool solved = (payload[1] & 0b01000000) != 0;
    
    game_module_update(id, ready, solved);
}

/*
 * Firmware - Module Strike - (0x12)
 * 
 * Packet Layout:
 * 
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Strikes      |                                               |   
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  */

/**
 * Send a module strike packet.
 */
void protocol_game_module_strike_send(uint8_t strikes) {
    uint8_t payload[2];
    
    payload[0] = OPCODE_GAME_MODULE_STRIKE;
    payload[1] = strikes;

    can_send(PREFIX_GAME, sizeof(payload), &payload[0]);
}

/**
 * Receive a game module strike packet, used to indicate a module has caused
 * a strike.
 * 
 * @param id CAN id for the source module, from the 8 least significant bits of the raw 11-bit CAN id
 * @param size size of CAN payload received
 * @param payload pointer to payload
 */
void protocol_game_module_strike_recieve(uint8_t id, uint8_t size, uint8_t *payload) {    
    /* Safety check. */
    if (size < 2) {
        return;
    }      
    
    uint8_t strikes = payload[1];
    
    game_strike_update(strikes);
}