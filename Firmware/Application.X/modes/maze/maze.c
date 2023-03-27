#include <xc.h>
#include "maze.h"
#include "../../tick.h"
#include "../../buzzer.h"
#include "../../rng.h"
#include "../../argb.h"
#include "../../mode.h"
#include "../../game.h"
#include "../../hal/pins.h"
#include "../../peripherals/keymatrix.h"

#define MAZE_RNG_MASK 0xb299a3d0

#define MAZE_COUNT  9
#define MAZE_SIZE   36

/* Local function pointers. */
uint8_t maze_map_array_to_argb(uint8_t array);
void maze_service(bool first);
void maze_service_idle(bool first);
void maze_service_setup(bool first);
void maze_service_running(bool first);
void maze_animate_cursor(uint8_t loc, bool beacon);
void maze_animate_destination(uint8_t loc, bool beacon);
void maze_disable(bool first);

/* Four bits per square, represent open paths for Up, Right, Down, Left. */
const uint8_t maze_layouts[MAZE_COUNT][18] = {
    {
        0b01100101, 0b00110110, 0b01010001,
        0b10100110, 0b10011100, 0b01010011,
        0b10101100, 0b00110110, 0b01011011,
        0b10100100, 0b11011001, 0b01001011,
        0b11100101, 0b00110110, 0b00011010,
        0b11000001, 0b11001001, 0b01001001,
    },
    {
        0b01000111, 0b00010110, 0b01110001,
        0b01101001, 0b01101001, 0b11000011,
        0b10100110, 0b10010110, 0b01011011,
        0b11101001, 0b01101001, 0b00101010,
        0b10100010, 0b10100110, 0b10011010,
        0b10001100, 0b10011100, 0b01011001,
    },
    {
        0b01100101, 0b00110010, 0b01100011,
        0b10000010, 0b10101100, 0b10011010,
        0b01101011, 0b10100110, 0b00111010,
        0b10101010, 0b10101010, 0b10101010,
        0b10101100, 0b10011010, 0b10101010,
        0b11000101, 0b01011001, 0b11001001,
    },
    {
        0b01100011, 0b01000101, 0b01010011,
        0b10101010, 0b01100101, 0b01011011,
        0b10101100, 0b10010110, 0b00011010,
        0b10100100, 0b01011101, 0b01011011,
        0b11100101, 0b01010101, 0b00111010,
        0b11000101, 0b00010100, 0b10011000,
    },
    {
        0b01000101, 0b01010101, 0b01110011,
        0b01100101, 0b01010111, 0b10011000,
        0b11100011, 0b01001001, 0b01100011,
        0b10101100, 0b01010011, 0b10001010,
        0b10100110, 0b01011101, 0b00011010,
        0b10001100, 0b01010101, 0b01011001,
    },
    {
        0b00100110, 0b00110100, 0b01110011,
        0b10101010, 0b10100110, 0b10011010,
        0b11101001, 0b10001010, 0b01101001,
        0b11000011, 0b01101011, 0b10100010,
        0b01101001, 0b10001010, 0b11001011,
        0b11000101, 0b01011001, 0b01001001,
    },
    {
        0b01100101, 0b01010011, 0b01100011,
        0b10100110, 0b00011100, 0b10011010,
        0b11001001, 0b01100001, 0b01101001,
        0b01100011, 0b11100101, 0b10010010,
        0b10101000, 0b11000101, 0b00111010,
        0b11000101, 0b01010101, 0b11011001,
    },
    {
        0b00100110, 0b01010011, 0b01100011,
        0b11101101, 0b00011100, 0b10011010,
        0b10100110, 0b01010101, 0b00111010,
        0b10101100, 0b00110100, 0b11011001,
        0b10100010, 0b11000101, 0b01010001,
        0b11001101, 0b01010101, 0b01010001,
    },
    {
        0b00100110, 0b01010101, 0b01110011,
        0b10101010, 0b01100001, 0b10101010,
        0b11101101, 0b10010110, 0b10011010,
        0b10100010, 0b01101001, 0b01001011,
        0b10101010, 0b10100110, 0b00111000,
        0b11001001, 0b11001001, 0b11000001,
    }
};

/* Location of the maze beacons, used by players to identify the maze layout. */
const uint8_t maze_beacons[MAZE_COUNT][2] = {
    {
        6,
        17,
    },
    {
        10,
        19,
    },
    {
        21,
        23,
    },
    {
        0,
        18,
    },
    {
        16,
        33,
    },
    {
        4,
        26,
    },
    {
        1,
        31,
    },
    {
        3,
        20,
    },
    {
        8,
        24,
    },
};

/* Keymatrix. */
pin_t maze_cols[] = {KPIN_A4, KPIN_A5, KPIN_A6, KPIN_A7, KPIN_NONE};
pin_t maze_rows[] = {KPIN_NONE};

/**
 * Initialise the maze mode.
 */
void maze_initialise(void) {
    /* Initialise ARGB expanded memory. */
    argb_expand(MAZE_ARGB_COUNT, &mode_data.maze.argb_leds[0], &mode_data.maze.argb_output[0]);
    
    /* Register game states. */
    mode_register_callback(GAME_ALWAYS, maze_service, NULL);
    mode_register_callback(GAME_IDLE, maze_service_idle, &tick_20hz);
    mode_register_callback(GAME_SETUP, maze_service_setup, &tick_20hz);
    mode_register_callback(GAME_RUNNING, maze_service_running, &tick_20hz);
    mode_register_callback(GAME_DISABLE, maze_disable, NULL);

    /* Initialise keymatrix. */
    keymatrix_initialise(&maze_cols[0], &maze_rows[0], KEYMODE_COL_ONLY);
}

void maze_disable(bool first) {
    for (uint8_t i = 0; i < MAZE_ARGB_COUNT; i++) {
        argb_set_module(i, 0, 0, 0);
    }
}

void maze_service_idle(bool first) {
    if (first) {
        maze_disable(first);
    }
}

/**
 * Service required peripherals for maze.
 */
void maze_service(bool first) {
    keymatrix_service();
}

/**
 * Set up the maze, choose the maze, start and finish positions.
 *
 * @param first true if first time the function is called
 */
void maze_service_setup(bool first) {
    if (first) {
        mode_data.maze.maze = rng_generate8(&game.module_seed, MAZE_RNG_MASK) % MAZE_COUNT;
        mode_data.maze.destination = rng_generate8(&game.module_seed, MAZE_RNG_MASK) % MAZE_SIZE;
        mode_data.maze.current = mode_data.maze.destination;

        /* Ensure that puzzle does not self solve. */
        while (mode_data.maze.current == mode_data.maze.destination) {
            mode_data.maze.current = rng_generate8(&game.module_seed, MAZE_RNG_MASK) % MAZE_SIZE;
        }

        game_module_ready(true);
    }
}

/**
 * Remap the position in the array to an argb position, this is needed due to
 * the argb matrix being a snake.
 *
 * @param array array position
 * @return argb position
 */
uint8_t maze_map_array_to_argb(uint8_t array) {
    uint8_t row = array / 6;

    if (row % 2 == 0) {
        return array;
    } else {
        return ((row + 1) * 6) - 1 - (array % 6);
    }
}

/**
 * Service the maze game, check for new input, check for solved and animate
 * display.
 *
 * @param first true if first time function is called
 */
void maze_service_running(bool first) {
    /* Throw away any pending button presses. */
    if (first) {
        keymatrix_clear();
    }

    if (this_module->solved) {
        return;
    }

    /* Handle moves. */
    for (uint8_t press = keymatrix_fetch(); press != KEY_NO_PRESS; press = keymatrix_fetch()) {
        if (press & KEY_DOWN_BIT) {
            /* Feedback to user button was accepted. */
            buzzer_on_timed(BUZZER_DEFAULT_VOLUME, BUZZER_FREQ_A6_SHARP, 40);

            /* Calculate the bits to check. */
            uint8_t i = mode_data.maze.current / 2;
            uint8_t permitted = maze_layouts[mode_data.maze.maze][i];

            if (mode_data.maze.current % 2 == 0) {
                permitted = permitted >> 4;
            }

            uint8_t check = 1 << 3 - (press & KEY_NUM_BITS);

            /* Verify if direction is valid to move. */
            if ((permitted & check) != 0) {
                uint8_t new_pos = mode_data.maze.current;
                switch(press & KEY_NUM_BITS) {
                    case 0: // Up
                        new_pos -=6;
                        break;
                    case 1: // Right
                        new_pos++;
                        break;
                    case 2: // Down
                        new_pos +=6;
                        break;
                    case 3: // Left
                        new_pos--;
                        break;
                }

                /* Sanity check new position, strike if it would be illegal. */
                if (new_pos > MAZE_SIZE) {
                    game_module_strike(1);
                } else {
                    mode_data.maze.current = new_pos;
                }
            } else {
                /* It was not, strike. */
                game_module_strike(1);
            }
        }
    }

    /* Check location, mark as solved if solved. */
    if (mode_data.maze.current == mode_data.maze.destination && !this_module->solved) {
        game_module_solved(true);
    }

    for (uint8_t i = 0; i < MAZE_SIZE; i++) {
        argb_set_module(i, 0, 0, 0);
    }

    if (!this_module->solved) {
        /* Draw the beacons regardless. */
        argb_set_module(maze_map_array_to_argb(maze_beacons[mode_data.maze.maze][0]), 0, 255, 0);
        argb_set_module(maze_map_array_to_argb(maze_beacons[mode_data.maze.maze][1]), 0, 255, 0);

        /* Draw the cursor. */
        maze_animate_cursor(mode_data.maze.current, (mode_data.maze.current == maze_beacons[mode_data.maze.maze][0] || mode_data.maze.current == maze_beacons[mode_data.maze.maze][1]));
        /* Draw the destination. */
        maze_animate_destination(mode_data.maze.destination, (mode_data.maze.destination == maze_beacons[mode_data.maze.maze][0] || mode_data.maze.destination == maze_beacons[mode_data.maze.maze][1]));
    }

    mode_data.maze.animation_frame++;

    if (mode_data.maze.animation_frame == 20) {
        mode_data.maze.animation_frame = 0;
    }
}

/**
 * Animate the players cursor in the maze, this cursor breaths white. If on
 * top of a beacon it will breath between white and green.
 *
 * @param loc cursor location
 * @param beacon true if on beacon
 */
void maze_animate_cursor(uint8_t loc, bool beacon) {
    uint8_t sec_g = 0;

    if (beacon) {
        sec_g = 255;
    }

    uint8_t i = mode_data.maze.animation_frame;
    if (i > 9) {
        i = 19 - i;
    }

    uint8_t r = (255 / 10) * i;
    uint8_t g = (255 / 10) * i;
    if (beacon) {
        g = 255;
    }
    uint8_t b = (255 / 10) * i;

    argb_set_module(maze_map_array_to_argb(loc), r, g, b);
}

/**
 * Animate the destination in the maze, this blinks red. If on top of a beacon
 * it will blink between red and green.
 *
 * @param loc destination location
 * @param beacon true if on beacon
 */
void maze_animate_destination(uint8_t loc, bool beacon) {
    if (mode_data.maze.animation_frame < 10) {
        argb_set_module(maze_map_array_to_argb(loc), 255, 0, 0);
    } else {
        if (beacon) {
            argb_set_module(maze_map_array_to_argb(loc), 0, 255, 0);
        }
    }
}