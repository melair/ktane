#ifndef KEYS_H
#define	KEYS_H

#include "../../argb.h"

void keys_initialise(void);

#define KEYS_MIN_INTERVAL 80
#define KEYS_MAX_INTERVAL 120
#define KEYS_LIGHT_WARNING 15

#define KEYS_ARGB_COUNT 7

typedef struct {
    uint8_t minutes;
    uint8_t seconds;
    uint8_t key;
    
    argb_led_t argb_leds[ARGB_MODULE_COUNT(KEYS_ARGB_COUNT)];
    uint8_t    argb_output[ARGB_BUFFER_SIZE(KEYS_ARGB_COUNT)];
} mode_keys_t;

#endif	/* KEYS_H */
