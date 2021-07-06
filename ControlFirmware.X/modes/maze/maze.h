#ifndef MAZE_H
#define	MAZE_H

void maze_initialise(void);
void maze_service(void);

typedef struct {
    uint32_t seed;
    
    uint8_t maze;    
    
    uint8_t current;
    uint8_t destination;
    
    uint8_t animation_frame;
} mode_maze_t;

#endif	/* MAZE_H */

