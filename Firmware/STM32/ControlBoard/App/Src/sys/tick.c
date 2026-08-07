#include "sys/tick.h"

#include "stm32h5xx_hal.h"

#define TICK_100HZ_PERIOD_MS 10U
#define TICK_20HZ_PERIOD_MS 50U
#define TICK_2HZ_PERIOD_MS 500U
#define TICK_1HZ_PERIOD_MS 1000U

volatile uint8_t tick_flags = 0;

typedef struct {
    uint32_t processed_tick_ms;
    uint16_t countdown_100hz;
    uint16_t countdown_20hz;
    uint16_t countdown_2hz;
    uint16_t countdown_1hz;
} tick_t;

static tick_t tick = {
    .countdown_100hz = TICK_100HZ_PERIOD_MS,
    .countdown_20hz = TICK_20HZ_PERIOD_MS,
    .countdown_2hz = TICK_2HZ_PERIOD_MS,
    .countdown_1hz = TICK_1HZ_PERIOD_MS,
};

static bool update_countdown(uint32_t elapsed_ms, uint16_t period_ms, uint16_t *countdown_ms) {
    if (elapsed_ms < *countdown_ms) {
        *countdown_ms -= (uint16_t) elapsed_ms;
        return false;
    }

    elapsed_ms -= *countdown_ms;
    *countdown_ms = (uint16_t) (period_ms - (elapsed_ms % period_ms));
    return true;
}

void tick_init(void) {
    tick_flags = 0;
    tick = (tick_t) {
        .processed_tick_ms = HAL_GetTick(),
        .countdown_100hz = TICK_100HZ_PERIOD_MS,
        .countdown_20hz = TICK_20HZ_PERIOD_MS,
        .countdown_2hz = TICK_2HZ_PERIOD_MS,
        .countdown_1hz = TICK_1HZ_PERIOD_MS,
    };
}

void tick_service_start(void) {
    const uint32_t current_tick_ms = HAL_GetTick();
    const uint32_t elapsed_ms = current_tick_ms - tick.processed_tick_ms;

    if (elapsed_ms == 0U) {
        return;
    }

    tick.processed_tick_ms = current_tick_ms;
    tick_flags |= TICK_1KHZ;

    if (update_countdown(elapsed_ms, TICK_100HZ_PERIOD_MS, &tick.countdown_100hz)) {
        tick_flags |= TICK_100HZ;
    }
    if (update_countdown(elapsed_ms, TICK_20HZ_PERIOD_MS, &tick.countdown_20hz)) {
        tick_flags |= TICK_20HZ;
    }
    if (update_countdown(elapsed_ms, TICK_2HZ_PERIOD_MS, &tick.countdown_2hz)) {
        tick_flags |= TICK_2HZ;
    }
    if (update_countdown(elapsed_ms, TICK_1HZ_PERIOD_MS, &tick.countdown_1hz)) {
        tick_flags |= TICK_1HZ;
    }
}

bool tick_service_end(void) {
    const bool tick_is_current = HAL_GetTick() == tick.processed_tick_ms;

    tick_flags = 0;
    return tick_is_current;
}
