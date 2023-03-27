#ifndef MAZE_H
#define	MAZE_H

#include <stdbool.h>
#include "../../argb.h"

void maze_initialise(void);

#define MAZE_ARGB_COUNT 36

typedef struct {
    uint8_t maze;

    uint8_t current;
    uint8_t destination;

    uint8_t animation_frame;
    
    argb_led_t argb_leds[ARGB_MODULE_COUNT(MAZE_ARGB_COUNT)];
    uint8_t    argb_output[ARGB_BUFFER_SIZE(MAZE_ARGB_COUNT)];
} mode_maze_t;

#endif	/* MAZE_H */

