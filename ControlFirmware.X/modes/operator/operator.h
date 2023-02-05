#ifndef OPERATOR_H
#define	OPERATOR_H

void operator_initialise(void);

#include <stdint.h>
#include <stdbool.h>

#define OPERATOR_LENGTH 5

typedef struct {        
    bool    rotary_dialing;
    uint8_t rotary_pulses;

    uint8_t dialed_numbers[OPERATOR_LENGTH];
    uint8_t dialed_pos;
    
    uint8_t wanted_numbers[OPERATOR_LENGTH];
    
    uint8_t offset_x;
    uint8_t offset_y;
} mode_operator_t;

#endif	/* OPERATOR_H */

