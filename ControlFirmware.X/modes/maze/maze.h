#ifndef MAZE_H
#define	MAZE_H

#include <stdbool.h>

void maze_initialise(void);

typedef struct {
    uint8_t maze;

    uint8_t current;
    uint8_t destination;

    uint8_t animation_frame;
} mode_maze_t;

#endif	/* MAZE_H */

