#include "pwm/pwm_platform.h"

#define PWM_TIMER_CLOCK_HZ 250000000U
#define PWM_FREQUENCY_HZ 1000U
#define PWM_TICK_HZ 1000000U
#define PWM_PRESCALER ((PWM_TIMER_CLOCK_HZ / PWM_TICK_HZ) - 1U)
#define PWM_PERIOD_TICKS (PWM_TICK_HZ / PWM_FREQUENCY_HZ)

typedef struct {
    TIM_TypeDef *instance;
    TIM_HandleTypeDef handle;
    void (*enable_clock)(void);
    bool initialized;
} PWM_Timer;

typedef struct {
    GPIO_PinDef pin;
    uint32_t alternate;
    PWM_Timer *timer;
    uint32_t channel;
    bool complementary;
    bool configured;
} PWM_Output;

static void tim1_clock_enable(void) {
    __HAL_RCC_TIM1_CLK_ENABLE();
}

static void tim2_clock_enable(void) {
    __HAL_RCC_TIM2_CLK_ENABLE();
}

static void tim3_clock_enable(void) {
    __HAL_RCC_TIM3_CLK_ENABLE();
}

static void tim5_clock_enable(void) {
    __HAL_RCC_TIM5_CLK_ENABLE();
}

static void tim12_clock_enable(void) {
    __HAL_RCC_TIM12_CLK_ENABLE();
}

static void tim17_clock_enable(void) {
    __HAL_RCC_TIM17_CLK_ENABLE();
}

static PWM_Timer tim1 = {.instance = TIM1, .enable_clock = tim1_clock_enable};
static PWM_Timer tim2 = {.instance = TIM2, .enable_clock = tim2_clock_enable};
static PWM_Timer tim3 = {.instance = TIM3, .enable_clock = tim3_clock_enable};
static PWM_Timer tim5 = {.instance = TIM5, .enable_clock = tim5_clock_enable};
static PWM_Timer tim12 = {.instance = TIM12, .enable_clock = tim12_clock_enable};
static PWM_Timer tim17 = {.instance = TIM17, .enable_clock = tim17_clock_enable};

static PWM_Output outputs[] = {
    {{GPIO_A0_Port, GPIO_A0_Pin}, GPIO_AF2_TIM12, &tim12, TIM_CHANNEL_2, false, false},
    {{GPIO_A1_Port, GPIO_A1_Pin}, GPIO_AF2_TIM12, &tim12, TIM_CHANNEL_1, false, false},
    {{GPIO_A4_Port, GPIO_A4_Pin}, GPIO_AF1_TIM1, &tim1, TIM_CHANNEL_2, false, false},
    {{GPIO_A5_Port, GPIO_A5_Pin}, GPIO_AF1_TIM1, &tim1, TIM_CHANNEL_2, true, false},
    {{GPIO_A6_Port, GPIO_A6_Pin}, GPIO_AF1_TIM1, &tim1, TIM_CHANNEL_1, false, false},
    {{GPIO_A7_Port, GPIO_A7_Pin}, GPIO_AF1_TIM1, &tim1, TIM_CHANNEL_1, true, false},
    {{GPIO_B2_Port, GPIO_B2_Pin}, GPIO_AF2_TIM3, &tim3, TIM_CHANNEL_1, false, false},
    {{GPIO_B3_Port, GPIO_B3_Pin}, GPIO_AF2_TIM3, &tim3, TIM_CHANNEL_2, false, false},
    {{GPIO_B4_Port, GPIO_B4_Pin}, GPIO_AF1_TIM2, &tim2, TIM_CHANNEL_4, false, false},
    {{GPIO_B6_Port, GPIO_B6_Pin}, GPIO_AF2_TIM3, &tim3, TIM_CHANNEL_3, false, false},
    {{GPIO_B7_Port, GPIO_B7_Pin}, GPIO_AF2_TIM3, &tim3, TIM_CHANNEL_4, false, false},
    {{GPIO_C0_Port, GPIO_C0_Pin}, GPIO_AF2_TIM5, &tim5, TIM_CHANNEL_1, false, false},
    {{GPIO_C1_Port, GPIO_C1_Pin}, GPIO_AF2_TIM5, &tim5, TIM_CHANNEL_2, false, false},
    {{GPIO_C2_Port, GPIO_C2_Pin}, GPIO_AF2_TIM5, &tim5, TIM_CHANNEL_3, false, false},
    {{GPIO_C3_Port, GPIO_C3_Pin}, GPIO_AF2_TIM5, &tim5, TIM_CHANNEL_4, false, false},
    {{GPIO_C6_Port, GPIO_C6_Pin}, GPIO_AF1_TIM17, &tim17, TIM_CHANNEL_1, false, false},
};

static PWM_Output *find_output(const GPIO_PinDef *pin) {
    if (pin == NULL) {
        return NULL;
    }

    for (uint8_t index = 0; index < sizeof(outputs) / sizeof(outputs[0]); index++) {
        PWM_Output *const output = &outputs[index];
        if (output->pin.port == pin->port && output->pin.pin == pin->pin) {
            return output;
        }
    }

    return NULL;
}

static bool timer_init(PWM_Timer *timer) {
    if (timer->initialized) {
        return true;
    }

    timer->enable_clock();
    timer->handle.Instance = timer->instance;
    timer->handle.Init.Prescaler = PWM_PRESCALER;
    timer->handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    timer->handle.Init.Period = PWM_PERIOD_TICKS - 1U;
    timer->handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    timer->handle.Init.RepetitionCounter = 0U;
    timer->handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

    if (HAL_TIM_PWM_Init(&timer->handle) != HAL_OK) {
        return false;
    }

    timer->initialized = true;
    return true;
}

static void pin_init(const PWM_Output *output) {
    GPIO_InitTypeDef gpio_init = {0};
    gpio_init.Pin = output->pin.pin;
    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    gpio_init.Alternate = output->alternate;
    HAL_GPIO_Init(output->pin.port, &gpio_init);
}

bool PWM_PlatformSetup(const GPIO_PinDef *pin) {
    PWM_Output *const output = find_output(pin);
    if (output == NULL) {
        return false;
    }

    if (output->configured) {
        return true;
    }

    if (!timer_init(output->timer)) {
        return false;
    }

    pin_init(output);

    TIM_OC_InitTypeDef channel_config = {0};
    channel_config.OCMode = TIM_OCMODE_PWM1;
    channel_config.Pulse = 0U;
    channel_config.OCPolarity = TIM_OCPOLARITY_HIGH;
    channel_config.OCNPolarity = TIM_OCNPOLARITY_HIGH;
    channel_config.OCFastMode = TIM_OCFAST_DISABLE;
    channel_config.OCIdleState = TIM_OCIDLESTATE_RESET;
    channel_config.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    if (HAL_TIM_PWM_ConfigChannel(&output->timer->handle, &channel_config, output->channel) != HAL_OK) {
        return false;
    }

    const HAL_StatusTypeDef start_status = output->complementary
                                               ? HAL_TIMEx_PWMN_Start(&output->timer->handle, output->channel)
                                               : HAL_TIM_PWM_Start(&output->timer->handle, output->channel);
    if (start_status != HAL_OK) {
        return false;
    }

    output->configured = true;
    return true;
}

void PWM_PlatformSetDuty(const GPIO_PinDef *pin, const uint8_t percent) {
    PWM_Output *const output = find_output(pin);
    if ((output == NULL) || !output->configured) {
        return;
    }

    const uint8_t clamped_percent = percent > 100U ? 100U : percent;
    const uint32_t compare = ((uint32_t) clamped_percent * PWM_PERIOD_TICKS) / 100U;
    __HAL_TIM_SET_COMPARE(&output->timer->handle, output->channel, compare);
}
