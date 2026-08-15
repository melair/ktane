#include "sys/mcu_load.h"

#include <stdint.h>

#include "stm32h5xx_hal.h"
#include "sys/sys_clock.h"

#define MCU_LOAD_SAMPLE_PERIOD_MS 1000U
#define MCU_LOAD_SCALE 10000U
#define MCU_LOAD_HISTORY_SIZE 60U

volatile uint16_t mcu_load_1s = 0;
volatile uint16_t mcu_load_5s = 0;
volatile uint16_t mcu_load_15s = 0;
volatile uint16_t mcu_load_60s = 0;

volatile uint16_t mcu_wakeups_1s = 0;
volatile uint16_t mcu_wakeups_5s = 0;
volatile uint16_t mcu_wakeups_15s = 0;
volatile uint16_t mcu_wakeups_60s = 0;

typedef struct {
    uint16_t load_history[MCU_LOAD_HISTORY_SIZE];
    uint16_t wakeups_history[MCU_LOAD_HISTORY_SIZE];
    uint8_t history_index;
    uint8_t history_count;
    uint32_t busy_us;
    uint32_t idle_us;
    uint32_t wakeups;
    uint32_t last_sample_ms;
    uint32_t period_start_us;
} mcu_load_t;

static mcu_load_t mcu_load = {0};

static uint16_t average(const uint16_t *history, uint8_t sample_count) {
    uint32_t sum = 0;

    if (sample_count > mcu_load.history_count) {
        sample_count = mcu_load.history_count;
    }

    if (sample_count == 0U) {
        return 0;
    }

    for (uint8_t i = 0; i < sample_count; i++) {
        const uint8_t sample_index = (uint8_t) ((MCU_LOAD_HISTORY_SIZE + mcu_load.history_index - 1U - i)
                                               % MCU_LOAD_HISTORY_SIZE);
        sum += history[sample_index];
    }

    return (uint16_t) ((sum + (sample_count / 2U)) / sample_count);
}

static void recordSample(uint16_t load, uint16_t wakeups) {
    mcu_load.load_history[mcu_load.history_index] = load;
    mcu_load.wakeups_history[mcu_load.history_index] = wakeups;
    mcu_load.history_index = (uint8_t) ((mcu_load.history_index + 1U) % MCU_LOAD_HISTORY_SIZE);

    if (mcu_load.history_count < MCU_LOAD_HISTORY_SIZE) {
        mcu_load.history_count++;
    }

    mcu_load_1s = average(mcu_load.load_history, 1U);
    mcu_load_5s = average(mcu_load.load_history, 5U);
    mcu_load_15s = average(mcu_load.load_history, 15U);
    mcu_load_60s = average(mcu_load.load_history, 60U);

    mcu_wakeups_1s = average(mcu_load.wakeups_history, 1U);
    mcu_wakeups_5s = average(mcu_load.wakeups_history, 5U);
    mcu_wakeups_15s = average(mcu_load.wakeups_history, 15U);
    mcu_wakeups_60s = average(mcu_load.wakeups_history, 60U);
}

static void calculate(void) {
    const uint32_t now_ms = HAL_GetTick();

    if (now_ms - mcu_load.last_sample_ms < MCU_LOAD_SAMPLE_PERIOD_MS) {
        return;
    }

    const uint32_t total_us = mcu_load.busy_us + mcu_load.idle_us;
    uint32_t load = 0;
    uint32_t wakeups = 0;

    if (total_us > 0U) {
        load = (uint32_t) ((((uint64_t) mcu_load.busy_us * MCU_LOAD_SCALE) + (total_us / 2U)) / total_us);
        if (load > MCU_LOAD_SCALE) {
            load = MCU_LOAD_SCALE;
        }
    }

    wakeups = ((mcu_load.wakeups * 1000U) + ((now_ms - mcu_load.last_sample_ms) / 2U)) /
              (now_ms - mcu_load.last_sample_ms);
    if (wakeups > UINT16_MAX) {
        wakeups = UINT16_MAX;
    }

    recordSample((uint16_t) load, (uint16_t) wakeups);

    mcu_load.busy_us = 0;
    mcu_load.idle_us = 0;
    mcu_load.wakeups = 0;
    mcu_load.last_sample_ms = now_ms;
}

void MCU_Load_Init(void) {
    mcu_load.last_sample_ms = HAL_GetTick();
    mcu_load.period_start_us = SysClock_GetUs();
}

void MCU_Load_Begin(void) {
    const uint32_t current_us = SysClock_GetUs();

    mcu_load.wakeups++;

    mcu_load.idle_us += current_us - mcu_load.period_start_us;
    mcu_load.period_start_us = current_us;
}

void MCU_Load_End(void) {
    const uint32_t busy_end_us = SysClock_GetUs();

    mcu_load.busy_us += busy_end_us - mcu_load.period_start_us;
    calculate();

    const uint32_t current_us = SysClock_GetUs();
    mcu_load.busy_us += current_us - busy_end_us;
    mcu_load.period_start_us = current_us;
}
