#ifndef TICK_H
#define TICK_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void tick_init(void);

void tick_service_start(void);

bool tick_service_end(void);

/* Tick flags bit-pack periodic events to reduce RAM usage. */
extern volatile uint8_t tick_flags;

#define TICK_1HZ   ((uint8_t) 0b00000001)
#define TICK_2HZ   ((uint8_t) 0b00000010)
#define TICK_20HZ  ((uint8_t) 0b00000100)
#define TICK_100HZ ((uint8_t) 0b00001000)
#define TICK_1KHZ  ((uint8_t) 0b00010000)

#define tick_1hz   ((tick_flags & TICK_1HZ) != 0U)
#define tick_2hz   ((tick_flags & TICK_2HZ) != 0U)
#define tick_20hz  ((tick_flags & TICK_20HZ) != 0U)
#define tick_100hz ((tick_flags & TICK_100HZ) != 0U)
#define tick_1khz  ((tick_flags & TICK_1KHZ) != 0U)

#ifdef __cplusplus
}
#endif

#endif /* TICK_H */
