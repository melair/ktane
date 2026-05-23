#ifndef MODE_DEFINITIONS_H
#define MODE_DEFINITIONS_H

#define MODE_COUNT 6
#define MODE_BLANK 0x00

#include "blank/blank.h"
#include "chassis/chassis.h"
#include "timer/timer.h"
#include "simon/simon.h"
#include "rfid/rfid.h"
#include "whosonfirst/whosonfirst.h"

const mode_t mode_definitions[MODE_COUNT] = {
    {.type = MODE_TYPE_BLANK, .id = 0x00, .name = "Blank", .state_funcs = &blank_funcs},
    {.type = MODE_TYPE_OTHER, .id = 0x00, .name = "Chassis Controller", .state_funcs = &chassis_funcs},
    {.type = MODE_TYPE_OTHER, .id = 0x01, .name = "Timer", .state_funcs = &timer_funcs},
    {.type = MODE_TYPE_PUZZLE, .id = 0x00, .name = "Simon Says", .state_funcs = &simon_funcs, .common_service = &simon_common_service},
    {.type = MODE_TYPE_PUZZLE, .id = 0x01, .name = "RFID", .state_funcs = &rfid_funcs},
    {.type = MODE_TYPE_PUZZLE, .id = 0x02, .name = "Who's On First", .state_funcs = &whosonfirst_funcs, .common_service = &whosonfirst_common_service},
};

#endif