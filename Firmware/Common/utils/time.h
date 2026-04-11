#ifndef TIME_H
#define TIME_H

#include <stdint.h>
#include <stdbool.h>

void time_init(void);
void time_interrupt(void);
void time_service_start(void);
bool time_service_end(void);

extern volatile uint32_t uptime_in_ms;
extern volatile uint8_t tick_flags;

#define TIME_TICK_1HZ    ((uint8_t)0b00000001)
#define TIME_TICK_2HZ    ((uint8_t)0b00000010)
#define TIME_TICK_20HZ   ((uint8_t)0b00000100)
#define TIME_TICK_100HZ  ((uint8_t)0b00001000)
#define TIME_TICK_1KHZ   ((uint8_t)0b00010000)
#define TIME_TICK_2KHZ   ((uint8_t)0b00100000)

#define tick_1hz   ((tick_flags & TIME_TICK_1HZ) != 0)
#define tick_2hz   ((tick_flags & TIME_TICK_2HZ) != 0)
#define tick_20hz  ((tick_flags & TIME_TICK_20HZ) != 0)
#define tick_100hz ((tick_flags & TIME_TICK_100HZ) != 0)
#define tick_1khz  ((tick_flags & TIME_TICK_1KHZ) != 0)
#define tick_2khz  ((tick_flags & TIME_TICK_2KHZ) != 0)

#endif
