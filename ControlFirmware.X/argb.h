#ifndef ARGB_H
#define	ARGB_H

#include <stdint.h>

void argb_initialise(void);
void argb_module_leds(uint8_t module_led_count);
void argb_set(uint8_t led, uint8_t bri, uint8_t r, uint8_t g, uint8_t b);
void argb_service(void);

#endif	/* ARGB_H */
