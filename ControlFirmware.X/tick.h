#ifndef TICK_H
#define	TICK_H

#include <stdint.h>

void tick_initialise(void);
void tick_interrupt(void);
uint24_t tick_fetch(void);
void tick_wait(uint8_t delay);

#endif	/* TICK_H */

