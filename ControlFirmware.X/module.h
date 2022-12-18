#ifndef MODULES_H
#define	MODULES_H

#include <stdbool.h>
#include "game.h"

void module_initialise(void);
void module_seen(uint8_t id, uint8_t mode, uint16_t firmware, uint32_t serial, uint8_t domain);
void module_error_record(uint8_t id, uint16_t code, bool active);
void module_error_raise(uint16_t code, bool active);
void module_service(void);
void module_errors_clear(uint8_t id);
void module_set_self_can_id(uint8_t id);
void module_set_self_domain(uint8_t domain);
module_game_t *module_get_game(uint8_t idx);
module_game_t *module_get_game_by_id(uint8_t id);

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

