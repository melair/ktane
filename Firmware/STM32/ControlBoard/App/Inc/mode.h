#ifndef MODE_H
#define MODE_H

#include "mode/puzzle/simon/simon.h"
#include "mode/support/chassis/chassis.h"
#include "mode/support/timer/timer.h"

#ifdef __cplusplus
extern "C" {

#endif

#define MODE_CFG(mode, id)      ((mode << 8) | id)

/* Mode Config Settings, 0x00XX. */
#define MODE_CONFIG_ACTIVE_MODE 0x00

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
} Module_Data;

extern Module_Data module_data;

#ifdef __cplusplus
}
#endif

#endif //MODE_H
