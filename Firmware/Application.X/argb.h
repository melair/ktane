#ifndef ARGB_H
#define	ARGB_H

#include <stdint.h>

/* Structure for storing the state of an ARGB LED. */
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} argb_led_t;

void argb_initialise(void);
void argb_expand(uint8_t count, argb_led_t *leds, uint8_t *output);
void argb_set_module(uint8_t led, uint8_t r, uint8_t g, uint8_t b);
void argb_set_status(uint8_t r, uint8_t g, uint8_t b);
void argb_status(uint8_t r, uint8_t g, uint8_t b);
void argb_service(void);
uint8_t argb_get_brightness(void);
void argb_set_brightness(uint8_t new_bri);

#define ARGB_MODULE_COUNT(count) (count + 1)
#define ARGB_BUFFER_SIZE(count) ((count + 1 + 2) * 4)

#endif	/* ARGB_H */
