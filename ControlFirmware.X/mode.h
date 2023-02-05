#ifndef MODE_H
#define	MODE_H

#include <stdbool.h>

#define MODE_COUNT 13

#define MODE_BLANK               0
#define MODE_BOOTSTRAP           1

#define MODE_CONTROLLER          3
#define MODE_CONTROLLER_STANDBY  4

#define MODE_PUZZLE_DEBUG        10
#define MODE_PUZZLE_MAZE         11
#define MODE_PUZZLE_SIMON        12
#define MODE_PUZZLE_PASSWORD     13
#define MODE_PUZZLE_WHOSONFIRST  14
#define MODE_PUZZLE_WIRES        15
#define MODE_PUZZLE_COMBINATION  16
#define MODE_PUZZLE_OPERATOR     17

#define MODE_NEEDY_KEYS          224

#define GAME_ALWAYS              0xff

uint8_t mode_get(void);
void mode_initialise(void);
void mode_service(void);
void mode_register_callback(uint8_t stage, void (*func)(bool), bool *tick);

#include "modes/controller/controller.h"
#include "modes/controller/ui.h"
#include "modes/maze/maze.h"
#include "modes/password/password.h"
#include "modes/simon/simon.h"
#include "modes/whosonfirst/whosonfirst.h"
#include "modes/wires/wires.h"
#include "modes/keys/keys.h"
#include "modes/combination/combination.h"
#include "modes/operator/operator.h"

typedef union {
    struct {
        mode_controller_control_t   ctrl;
        mode_controller_ui_t        ui;
    } controller;
    mode_maze_t         maze;
    mode_password_t     password;
    mode_simon_t        simon;
    mode_whosonfirst_t  whosonfirst;
    mode_wires_t        wires;
    mode_keys_t         keys;
    mode_combination_t  combination;
    mode_operator_t     operator;
} mode_data_t;

extern mode_data_t mode_data;

#define MAX_NAME 20

typedef struct {
    uint8_t id;
    uint8_t name[MAX_NAME];
} mode_names_t;

extern mode_names_t mode_names[];

#endif	/* MODE_H */

