#ifndef PWMLED_H
#define	PWMLED_H

#include <stdbool.h>
#include "ports.h"

void pwmled_initialise(pin_t r, pin_t g, pin_t b, pin_t *ch);
void pwmled_service(void);
void pwmled_set(uint8_t ch, uint8_t bri, uint8_t r, uint8_t g, uint8_t b);

#endif	/* PWMLED_H */
