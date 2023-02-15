#ifndef CONTROLLER_H
#define	CONTROLLER_H

#include <stdbool.h>
#include "../../argb.h"

#define CONTROLLER_ARGB_COUNT 7

typedef struct {
    argb_led_t argb_leds[ARGB_MODULE_COUNT(CONTROLLER_ARGB_COUNT)];
    uint8_t    argb_output[ARGB_BUFFER_SIZE(CONTROLLER_ARGB_COUNT)];
} mode_controller_control_t;

void controller_initialise(void);

#endif	/* CONTROLLER_H */
