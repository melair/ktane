#ifndef TIME_H
#define TIME_H

#include <stdint.h>
#include <stdbool.h>

void time_init(void);
void time_interrupt(void);
void time_service_start(void);
bool time_service_end(void);

extern volatile uint32_t uptime;
extern volatile bool tick_1hz;
extern volatile bool tick_2hz;
extern volatile bool tick_20hz;
extern volatile bool tick_100hz;
extern volatile bool tick_1khz;
extern volatile bool tick_2khz;

#endif