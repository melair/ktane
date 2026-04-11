#ifndef ARGB_H
#define ARGB_H

#include <stdint.h>
#include <stdbool.h>
#include "pin.h"

typedef struct {
    uint8_t G;
    uint8_t R;
    uint8_t B;
} argb_led_t;

#define ARGB_DEFAULT_BUFFER_SIZE 1

void argb_init(pin_t out, bool negate);
void argb_service(void);
void argb_interrupt(void);
void argb_set_buffer(argb_led_t *buffer, uint8_t len);
void argb_set(uint8_t idx, uint8_t r, uint8_t g, uint8_t b);
void argb_update(void);

#endif