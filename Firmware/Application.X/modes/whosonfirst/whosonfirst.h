#ifndef WHOSONFIRST_H
#define	WHOSONFIRST_H

#include "../../argb.h"

void whosonfirst_initialise(void);

#define WHOSONFIRST_WORD_COUNT 6

#define WHOSONFIRST_ARGB_COUNT 6

typedef struct {
    uint8_t stage;

    uint8_t topword;
    uint8_t words[WHOSONFIRST_WORD_COUNT];
       
    argb_led_t argb_leds[ARGB_MODULE_COUNT(WHOSONFIRST_ARGB_COUNT)];
    uint8_t    argb_output[ARGB_BUFFER_SIZE(WHOSONFIRST_ARGB_COUNT)];
} mode_whosonfirst_t;

#endif	/* WHOSONFIRST_H */

