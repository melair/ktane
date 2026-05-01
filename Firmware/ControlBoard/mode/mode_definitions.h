#ifndef MODE_DEFINITIONS_H
#define MODE_DEFINITIONS_H

#include "mode.h"

#define MODE_COUNT 6

#define MODE_BLANK 0x00
#include "blank/blank.h"

const mode_t mode_definitions[MODE_COUNT] = {
    {.type = MODE_TYPE_BLANK, .id = 0x00, .name = "Blank", .state_funcs = &blank_funcs[0]},
    {.type = MODE_TYPE_OTHER, .id = 0x00, .name = "Chassis Controller"},
    {.type = MODE_TYPE_OTHER, .id = 0x01, .name = "Timer"},
    {.type = MODE_TYPE_PUZZLE, .id = 0x00, .name = "Simon Says"},
    {.type = MODE_TYPE_PUZZLE, .id = 0x01, .name = "RFID"},
    {.type = MODE_TYPE_PUZZLE, .id = 0x02, .name = "Who's On First"},
};

#endif
