#ifndef MODE_DEFINITIONS_H
#define MODE_DEFINITIONS_H

#include "mode.h"

#define MODE_COUNT 6

#define MODE_BLANK 0x00

#include "blank/blank.h"
#include "chassis/chassis.h"
#include "timer/timer.h"
#include "simon/simon.h"
#include "rfid/rfid.h"
#include "whosonfirst/whosonfirst.h"

const mode_t mode_definitions[MODE_COUNT] = {
    {.type = MODE_TYPE_BLANK, .id = 0x00, .name = "Blank", .state_funcs = &blank_funcs[0]},
    {.type = MODE_TYPE_OTHER, .id = 0x00, .name = "Chassis Controller", .state_funcs = &chassis_funcs[0]},
    {.type = MODE_TYPE_OTHER, .id = 0x01, .name = "Timer", .state_funcs = &timer_funcs[0]},
    {.type = MODE_TYPE_PUZZLE, .id = 0x00, .name = "Simon Says", .state_funcs = &simon_funcs[0]},
    {.type = MODE_TYPE_PUZZLE, .id = 0x01, .name = "RFID", .state_funcs = &rfid_funcs[0]},
    {.type = MODE_TYPE_PUZZLE, .id = 0x02, .name = "Who's On First", .state_funcs = &whosonfirst_funcs[0]},
};

#endif
