#include "lamp/lamp.h"

/* Same signature across STM32 HAL families; avoid family-specific headers. */
extern uint32_t HAL_GetTick(void);

void Lamp_Init(Lamp *lamp) {
    lamp->last_tick_ms = HAL_GetTick();
    lamp->level = 0U;
    lamp->remainder = 0U;
    lamp->on = false;
}

void Lamp_Set(Lamp *lamp, bool on) {
    Lamp_Service(lamp);
    if (lamp->on != on) {
        lamp->remainder = 0U;
        lamp->on = on;
    }
}

void Lamp_Service(Lamp *lamp) {
    const uint32_t now = HAL_GetTick();
    const uint32_t elapsed_ms = now - lamp->last_tick_ms;
    lamp->last_tick_ms = now;

    const uint32_t duration_ms = lamp->on ? LAMP_FADE_IN_MS : LAMP_FADE_OUT_MS;
    const uint16_t target = lamp->on ? UINT16_MAX : 0U;
    if (lamp->level == target || elapsed_ms >= duration_ms) {
        lamp->level = target;
        lamp->remainder = 0U;
        return;
    }

    /* The duration bound and long-delay shortcut keep this within 32 bits.
     * Carry division remainder so small, frequent updates retain progress.
     */
    const uint32_t progress = elapsed_ms * UINT16_MAX + lamp->remainder;
    const uint32_t step = progress / duration_ms;
    const uint32_t distance = lamp->on ? UINT16_MAX - lamp->level : lamp->level;
    if (step >= distance) {
        lamp->level = target;
        lamp->remainder = 0U;
    } else {
        lamp->level = (uint16_t) (lamp->on ? lamp->level + step : lamp->level - step);
        lamp->remainder = (uint16_t) (progress % duration_ms);
    }
}

uint8_t Lamp_GetPWM(const Lamp *lamp) {
    /* Squared fade gives a gentle start and a dim tail. Scaling by
     * 65535^2 / 255 is exact; round via the remainder to avoid overflow
     * from adding half the divisor to a near-maximum squared level.
     */
    const uint32_t scale = ((uint32_t) UINT16_MAX * UINT16_MAX) / UINT8_MAX;
    const uint32_t squared = (uint32_t) lamp->level * lamp->level;
    return (uint8_t) (squared / scale + (squared % scale > scale / 2U));
}
