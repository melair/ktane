#ifndef MODULES_H
#define	MODULES_H

#include <stdbool.h>

void modules_initialise(void);
void module_seen(uint8_t id, uint8_t mode, uint16_t firmware);
void module_error_record(uint8_t id, uint16_t code);
void module_error_raise(uint16_t code);
void module_service(void);
bool modules_no_errors(void);

#define MODULE_ERROR_NONE                       0x0000
#define MODULE_ERROR_CAN_ID_CONFLICT            0x0010

#define MODULE_ERROR_FIRMWARE_START             0x0020
#define MODULE_ERROR_FIRMWARE_FAILED            0x0021

#define MODULE_ERROR_CAN_LOST_BASE              0xe000 // 0xe000 to 0xe0ff

#define MODULE_ERROR_PROTOCOL_UNKNOWN           0xf800 // 0xf800 to 0xfffe

#define MODULE_ERROR_TOO_MANY_ERRORS            0xffff

#endif	/* MODULES_H */

