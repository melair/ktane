#ifndef TICK_H
#define	TICK_H

#include <stdint.h>
#include <stdbool.h>

void tick_initialise(void);
void tick_wait(uint8_t delay);
void tick_service(void);

extern volatile uint32_t tick_value;
extern volatile bool tick_2hz;
extern volatile bool tick_20hz;
extern volatile bool tick_100hz;
extern volatile bool tick_1khz;
extern volatile bool tick_2khz;

#endif	/* TICK_H */

