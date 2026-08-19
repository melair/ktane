#include "sys/input_manager_config.h"

static TIM_HandleTypeDef module_a_timer;
static TIM_HandleTypeDef module_b_timer;
static TIM_HandleTypeDef module_c_timer;

static void module_a_timer_clock_enable(void) {
    __HAL_RCC_TIM1_CLK_ENABLE();
}

static void module_b_timer_clock_enable(void) {
    __HAL_RCC_TIM3_CLK_ENABLE();
}

static void module_c_timer_clock_enable(void) {
    __HAL_RCC_TIM5_CLK_ENABLE();
}

const IM_RotaryHardware CONTROL_BOARD_ROTARY_MODULE_A = {
    .timer_instance = TIM1,
    .timer_handle = &module_a_timer,
    .channel_a = {GPIO_A6_Port, GPIO_A6_Pin},
    .channel_a_alternate = GPIO_AF1_TIM1,
    .channel_b = {GPIO_A4_Port, GPIO_A4_Pin},
    .channel_b_alternate = GPIO_AF1_TIM1,
    .enable_timer_clock = module_a_timer_clock_enable,
};

const IM_RotaryHardware CONTROL_BOARD_ROTARY_MODULE_B = {
    .timer_instance = TIM3,
    .timer_handle = &module_b_timer,
    .channel_a = {GPIO_B2_Port, GPIO_B2_Pin},
    .channel_a_alternate = GPIO_AF2_TIM3,
    .channel_b = {GPIO_B3_Port, GPIO_B3_Pin},
    .channel_b_alternate = GPIO_AF2_TIM3,
    .enable_timer_clock = module_b_timer_clock_enable,
};

const IM_RotaryHardware CONTROL_BOARD_ROTARY_MODULE_C = {
    .timer_instance = TIM5,
    .timer_handle = &module_c_timer,
    .channel_a = {GPIO_C0_Port, GPIO_C0_Pin},
    .channel_a_alternate = GPIO_AF2_TIM5,
    .channel_b = {GPIO_C1_Port, GPIO_C1_Pin},
    .channel_b_alternate = GPIO_AF2_TIM5,
    .enable_timer_clock = module_c_timer_clock_enable,
};
