#ifndef TICK_H
#define	TICK_H

#include <stdint.h>

void tick_initialise(void);
void tick_wait(uint8_t delay);
void tick_service(void);

extern volatile uint32_t tick_value;

#endif	/* TICK_H */

