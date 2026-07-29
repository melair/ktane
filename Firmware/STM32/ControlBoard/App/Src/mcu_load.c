#include "mcu_load.h"

#include <stdint.h>

#include "main.h"
#include "stm32h5xx_it.h"

#define MCU_LOAD_TIM6_CLOCK_HZ 250000000U
#define MCU_LOAD_TIMER_HZ 200000U
#define MCU_LOAD_TIMER_PRESCALER ((MCU_LOAD_TIM6_CLOCK_HZ / MCU_LOAD_TIMER_HZ) - 1U)
#define MCU_LOAD_SAMPLE_PERIOD_MS 1000U
#define MCU_LOAD_SCALE 10000U
#define MCU_LOAD_HISTORY_SIZE 60U

/* TIM6 is clocked at 250 MHz. Prescaler 1249 gives a 200 kHz timer, so each tick is 5 us. */

volatile uint16_t mcu_load_1s = 0;
volatile uint16_t mcu_load_5s = 0;
volatile uint16_t mcu_load_15s = 0;
volatile uint16_t mcu_load_60s = 0;

volatile uint16_t mcu_wakeups_1s = 0;
volatile uint16_t mcu_wakeups_5s = 0;
volatile uint16_t mcu_wakeups_15s = 0;
volatile uint16_t mcu_wakeups_60s = 0;

typedef struct {
    TIM_HandleTypeDef htim6;
    uint16_t load_history[MCU_LOAD_HISTORY_SIZE];
    uint16_t wakeups_history[MCU_LOAD_HISTORY_SIZE];
    uint8_t history_index;
    uint8_t history_count;
    uint32_t busy_ticks;
    uint32_t idle_ticks;
    uint32_t wakeups;
    uint32_t last_sample_ms;
    uint16_t period_start_tick;
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

    const uint32_t total_ticks = mcu_load.busy_ticks + mcu_load.idle_ticks;
    uint32_t load = 0;
    uint32_t wakeups = 0;

    if (total_ticks > 0U) {
        load = ((mcu_load.busy_ticks * MCU_LOAD_SCALE) + (total_ticks / 2U)) / total_ticks;
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

    mcu_load.busy_ticks = 0;
    mcu_load.idle_ticks = 0;
    mcu_load.wakeups = 0;
    mcu_load.last_sample_ms = now_ms;
}

void MCU_Load_Init(void) {
    TIM_ClockConfigTypeDef clock_source_config = {0};
    TIM_MasterConfigTypeDef master_config = {0};

    __HAL_RCC_TIM6_CLK_ENABLE();

    mcu_load.htim6.Instance = TIM6;
    mcu_load.htim6.Init.Prescaler = MCU_LOAD_TIMER_PRESCALER;
    mcu_load.htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
    mcu_load.htim6.Init.Period = UINT16_MAX;
    mcu_load.htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&mcu_load.htim6) != HAL_OK) {
        Error_Handler();
    }

    clock_source_config.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&mcu_load.htim6, &clock_source_config) != HAL_OK) {
        Error_Handler();
    }

    master_config.MasterOutputTrigger = TIM_TRGO_RESET;
    master_config.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&mcu_load.htim6, &master_config) != HAL_OK) {
        Error_Handler();
    }

    __HAL_TIM_SET_COUNTER(&mcu_load.htim6, 0U);
    if (HAL_TIM_Base_Start(&mcu_load.htim6) != HAL_OK) {
        Error_Handler();
    }

    mcu_load.last_sample_ms = HAL_GetTick();
    mcu_load.period_start_tick = (uint16_t) __HAL_TIM_GET_COUNTER(&mcu_load.htim6);
}

void MCU_Load_Begin(void) {
    const uint16_t current_tick = (uint16_t) __HAL_TIM_GET_COUNTER(&mcu_load.htim6);

    mcu_load.wakeups++;

    mcu_load.idle_ticks += (uint16_t) (current_tick - mcu_load.period_start_tick);
    mcu_load.period_start_tick = current_tick;
}

void MCU_Load_End(void) {
    const uint16_t busy_end_tick = (uint16_t) __HAL_TIM_GET_COUNTER(&mcu_load.htim6);

    mcu_load.busy_ticks += (uint16_t) (busy_end_tick - mcu_load.period_start_tick);
    calculate();

    const uint16_t current_tick = (uint16_t) __HAL_TIM_GET_COUNTER(&mcu_load.htim6);
    mcu_load.busy_ticks += (uint16_t) (current_tick - busy_end_tick);
    mcu_load.period_start_tick = current_tick;
}
