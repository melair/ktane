#ifndef MODULES_H
#define	MODULES_H

#include <stdbool.h>
#include "game.h"

/* Number of errors in each module to track. */
#define ERROR_COUNT  8

/* Structure for an error received from a module. */
typedef struct {
    uint16_t code;

    struct {
        unsigned count : 7;
        unsigned active : 1;
    };
} module_error_t;

/* Structure for a module. */
typedef struct {

    struct {
        unsigned INUSE : 1;
        unsigned LOST : 1;
        unsigned DEBUG : 1;
    } flags;

    uint8_t id;
    uint8_t mode;
    uint32_t last_seen;
    uint32_t serial;

    struct {
        uint16_t bootloader;
        uint16_t application;
        uint16_t flasher;
    } firmware;

    uint8_t opts[3];

    module_error_t errors[ERROR_COUNT];
    module_game_t game;
} module_t;

void module_initialise(void);
void module_error_raise(uint16_t code, bool active);
void module_service(void);
void module_set_self_can_id(uint8_t id);
module_game_t *module_get_game(uint8_t idx);
module_game_t *module_get_game_by_id(uint8_t id);
module_t *module_get(uint8_t idx);
module_error_t *module_get_errors(uint8_t idx, uint8_t err);

uint8_t module_get_count_enabled_module(void);
uint8_t module_get_count_enabled_puzzle(void);
uint8_t module_get_count_enabled_solved_puzzle(void);
uint8_t module_get_count_enabled_needy(void);

void module_send_reset(void);
void module_send_identify(uint8_t id);
void module_send_mode_set(uint8_t id, uint8_t mode);
void module_send_special_function(uint8_t id, uint8_t special_fn);
void module_send_global_config(bool store);

#include "../common/packet.h"

void module_receive_announce(uint8_t id, packet_t *p);
void module_receive_error(uint8_t id, packet_t *p);
void module_receive_reset(uint8_t id, packet_t *p);
void module_receive_identify(uint8_t id, packet_t *p);
void module_receive_mode_set(uint8_t id, packet_t *p);
void module_receive_special_function(uint8_t id, packet_t *p);
void module_receive_global_config(uint8_t id, packet_t *p);

/* Total number of modules that can be part of the network. */
#define MODULE_COUNT 24

#define MODULE_ERROR_NONE                       0x0000
#define MODULE_ERROR_CAN_ID_CONFLICT            0x0010

#define MODULE_ERROR_FIRMWARE_START             0x0020
#define MODULE_ERROR_FIRMWARE_FAILED            0x0021

#define MODULE_ERROR_CAN_LOST_BASE              0xe000 // 0xe000 to 0xe0ff

#define MODULE_ERROR_PROTOCOL_UNKNOWN           0xf800 // 0xf800 to 0xfffe

#define MODULE_ERROR_TOO_MANY_ERRORS            0xffff

#endif	/* MODULES_H */

