#ifndef OPERATOR_H
#define	OPERATOR_H

void operator_initialise(void);

#include <stdint.h>
#include <stdbool.h>
#include "../../argb.h"

#define OPERATOR_LENGTH 5

#define OPERATOR_ARGB_COUNT 6

typedef struct {
    bool    rotary_dialing;
    uint8_t rotary_pulses;

    uint8_t dialed_numbers[OPERATOR_LENGTH];
    uint8_t dialed_pos;

    uint8_t wanted_numbers[OPERATOR_LENGTH];

    uint8_t offset_x;
    uint8_t offset_y;
    
    argb_led_t argb_leds[ARGB_MODULE_COUNT(OPERATOR_ARGB_COUNT)];
    uint8_t    argb_output[ARGB_BUFFER_SIZE(OPERATOR_ARGB_COUNT)];
} mode_operator_t;

#endif	/* OPERATOR_H */

