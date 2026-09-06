#ifndef MODE_H
#define MODE_H

#include <stdbool.h>
#include <stdint.h>
#include "node_link/protocol.h"
#include "mode_fsm.h"
#include "mode/puzzle/simon/simon.h"
#include "mode/support/chassis/chassis.h"
#include "mode/support/timer/timer.h"

#ifdef __cplusplus
extern "C" {

#endif

#define MODE_CFG(mode, id)      ((mode << 8) | id)

/* Global Config Settings, 0x00XX. */
#define MODE_CONFIG_ACTIVE_MODE 0x00
#define MODE_CONFIG_CAN_IDENTIFIER 0x01

#define MODE_TYPE_SHIFT         6
#define MODE_TYPE_MASK          (0x03 << MODE_TYPE_SHIFT)

#define MODE_TYPE_NONE          0x00
#define MODE_TYPE_PUZZLE        (0x01 << MODE_TYPE_SHIFT)
#define MODE_TYPE_NEEDY         (0x02 << MODE_TYPE_SHIFT)
#define MODE_TYPE_SUPPORT       (0x03 << MODE_TYPE_SHIFT)

/* Other Modules */
#define MODE_NONE               (MODE_TYPE_NONE | 0x00)

/* Support Modules */
#define MODE_SUPPORT_CHASSIS    (MODE_TYPE_SUPPORT | 0x00)
#define MODE_SUPPORT_TIMER      (MODE_TYPE_SUPPORT | 0x01)

/* Puzzle Modules */
#define MODE_PUZZLE_SIMON       (MODE_TYPE_PUZZLE | 0x00)

/* Needy Modules */

/* Structures */
typedef struct {
    union {
        Simon_Data simon;
        Chassis_Data chassis;
        Timer_Data timer;
    } mode;
} Mode_Data;

extern Mode_Data mode_data;

void Mode_Init(void);

void Mode_Service(void);

void Mode_SetServiceEnabled(bool enabled);

uint8_t Mode_Get(void);

void Mode_Set(uint8_t mode);

#ifdef __cplusplus
}
#endif

#endif //MODE_H
