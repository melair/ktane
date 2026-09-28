#ifndef LAMP_H
#define LAMP_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Shared visual fade durations for a complete transition, in milliseconds.
 * Tune within 1..65535 and rebuild. Partial fades take proportionally less time.
 */
#define LAMP_FADE_IN_MS 50U
#define LAMP_FADE_OUT_MS 200U

typedef struct {
    uint32_t last_tick_ms;
    uint16_t level;      /* Linear fade position: 0..65535. */
    uint16_t remainder;  /* Fractional progress retained between service calls. */
    bool on;
} Lamp;

/* All functions require valid, initialised state (except Init) and serialised
 * access. Elapsed intervals must be shorter than one HAL tick rollover period.
 */
void Lamp_Init(Lamp *lamp);

/* Advance under the previous command before changing it; retain fade level. */
void Lamp_Set(Lamp *lamp, bool on);

/* Nominally called every 50 ms; advances by actual HAL_GetTick() elapsed time. */
void Lamp_Service(Lamp *lamp);

/* Read current brightness without advancing time. 0 = off, 255 = fully on.
 * Apply directly as LED PWM duty, without additional gamma correction.
 */
uint8_t Lamp_GetPWM(const Lamp *lamp);

#ifdef __cplusplus
}
#endif

#endif // LAMP_H
