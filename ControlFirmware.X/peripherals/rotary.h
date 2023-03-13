#ifndef ROTARY_H
#define	ROTARY_H

#include "../hal/pins.h"

void rotary_initialise(pin_t a, pin_t b);
void rotary_service(void);
int8_t rotary_fetch_delta(void);
void rotary_clear(void);

#endif	/* ROTARY_H */

