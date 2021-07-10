#ifndef MODE_H
#define	MODE_H

#include <stdbool.h>

#define MODE_BLANK               0
#define MODE_BOOTSTRAP           1
#define MODE_UNCONFIGURED        2

#define MODE_CONTROLLER          3
#define MODE_CONTROLLER_STANDBY  4

#define MODE_PUZZLE_DEBUG        10
#define MODE_PUZZLE_MAZE         11
#define MODE_PUZZLE_SIMON        12
#define MODE_PUZZLE_PASSWORD     13

#define GAME_ALWAYS              0xff

uint8_t mode_get(void);
void mode_initialise(void);
void mode_service(void);
void mode_register_callback(uint8_t stage, void (*func)(bool));

#include "modes/maze/maze.h"
#include "modes/password/password.h"

typedef union {
    mode_maze_t     maze;
    mode_password_t password;
} mode_data_t;

extern mode_data_t mode_data;

#endif	/* MODE_H */

