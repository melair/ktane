#ifndef SIMON_H
#define	SIMON_H

void simon_initialise(void);

typedef struct {        
    uint8_t count;
    uint16_t order;
    
    uint32_t next_animate;
    
    uint8_t next_correct_press;
    uint8_t next_highest_press;
    
    uint8_t next_display;
    uint8_t next_display_stage;
} mode_simon_t;

#endif	/* SIMON_H */

