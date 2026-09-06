#include "input_manager/input_manager_platform.h"

#define IM_ROTARY_COUNTER_PERIOD 0xFFFFu

static ADC_HandleTypeDef adc;

static bool analogue_init(void) {
    __HAL_RCC_ADCDAC_CONFIG(RCC_ADCDACCLKSOURCE_PLL2R);
    __HAL_RCC_ADC_CLK_ENABLE();

    adc.Instance = ADC1;
    adc.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
    adc.Init.Resolution = ADC_RESOLUTION_12B;
    adc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    adc.Init.ScanConvMode = ADC_SCAN_DISABLE;
    adc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    adc.Init.LowPowerAutoWait = DISABLE;
    adc.Init.ContinuousConvMode = DISABLE;
    adc.Init.NbrOfConversion = 1;
    adc.Init.DiscontinuousConvMode = DISABLE;
    adc.Init.NbrOfDiscConversion = 1;
    adc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    adc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    adc.Init.SamplingMode = ADC_SAMPLING_MODE_NORMAL;
    adc.Init.DMAContinuousRequests = DISABLE;
    adc.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
    adc.Init.OversamplingMode = DISABLE;

    if (HAL_ADC_Init(&adc) != HAL_OK) {
        return false;
    }

    return HAL_ADCEx_Calibration_Start(&adc, ADC_SINGLE_ENDED) == HAL_OK;
}

static bool analogue_start(uint32_t adc_channel) {
    ADC_ChannelConfTypeDef channel_config = {0};

    channel_config.Channel = adc_channel;
    channel_config.Rank = ADC_REGULAR_RANK_1;
    channel_config.SamplingTime = ADC_SAMPLETIME_47CYCLES_5;
    channel_config.SingleDiff = ADC_SINGLE_ENDED;
    channel_config.OffsetNumber = ADC_OFFSET_NONE;
    channel_config.Offset = 0;

    if (HAL_ADC_ConfigChannel(&adc, &channel_config) != HAL_OK) {
        return false;
    }

    return HAL_ADC_Start(&adc) == HAL_OK;
}

static bool analogue_poll(uint16_t *value) {
    if (HAL_ADC_PollForConversion(&adc, 0) != HAL_OK) {
        return false;
    }

    *value = (uint16_t) HAL_ADC_GetValue(&adc);
    HAL_ADC_Stop(&adc);
    return true;
}

static void rotary_pin_configure(const GPIO_PinDef *pin, uint32_t alternate, bool enable_internal_pullups) {
    GPIO_InitTypeDef gpio_init = {0};

    gpio_init.Pin = pin->pin;
    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Pull = enable_internal_pullups ? GPIO_PULLUP : GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    gpio_init.Alternate = alternate;
    HAL_GPIO_Init(pin->port, &gpio_init);
}

static bool rotary_init(const IM_RotaryHardware *hardware, bool enable_internal_pullups) {
    TIM_Encoder_InitTypeDef encoder_config = {0};
    TIM_MasterConfigTypeDef master_config = {0};

    if ((hardware == NULL) || (hardware->timer_instance == NULL) || (hardware->timer_handle == NULL) ||
        (hardware->channel_a.port == NULL) || (hardware->channel_b.port == NULL) ||
        (hardware->enable_timer_clock == NULL)) {
        return false;
    }

    TIM_HandleTypeDef *handle = hardware->timer_handle;

    hardware->enable_timer_clock();
    rotary_pin_configure(&hardware->channel_a, hardware->channel_a_alternate, enable_internal_pullups);
    rotary_pin_configure(&hardware->channel_b, hardware->channel_b_alternate, enable_internal_pullups);

    handle->Instance = hardware->timer_instance;
    handle->Init.Prescaler = 0;
    handle->Init.CounterMode = TIM_COUNTERMODE_UP;
    handle->Init.Period = IM_ROTARY_COUNTER_PERIOD;
    handle->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    handle->Init.RepetitionCounter = 0;
    handle->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    encoder_config.EncoderMode = TIM_ENCODERMODE_TI12;
    encoder_config.IC1Polarity = TIM_ICPOLARITY_RISING;
    encoder_config.IC1Selection = TIM_ICSELECTION_DIRECTTI;
    encoder_config.IC1Prescaler = TIM_ICPSC_DIV1;
    encoder_config.IC1Filter = 15;
    encoder_config.IC2Polarity = TIM_ICPOLARITY_RISING;
    encoder_config.IC2Selection = TIM_ICSELECTION_DIRECTTI;
    encoder_config.IC2Prescaler = TIM_ICPSC_DIV1;
    encoder_config.IC2Filter = 15;

    if (HAL_TIM_Encoder_Init(handle, &encoder_config) != HAL_OK) {
        return false;
    }

    master_config.MasterOutputTrigger = TIM_TRGO_RESET;
    master_config.MasterOutputTrigger2 = TIM_TRGO2_RESET;
    master_config.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(handle, &master_config) != HAL_OK) {
        return false;
    }

    __HAL_TIM_SET_COUNTER(handle, 0);
    return HAL_TIM_Encoder_Start(handle, TIM_CHANNEL_ALL) == HAL_OK;
}

static uint16_t rotary_count(const IM_RotaryHardware *hardware) {
    if ((hardware == NULL) || (hardware->timer_handle == NULL)) {
        return 0;
    }

    const TIM_HandleTypeDef *handle = hardware->timer_handle;
    return (uint16_t) __HAL_TIM_GET_COUNTER(handle);
}

const IM_Platform IM_PLATFORM = {
    .analogue_init = analogue_init,
    .analogue_start = analogue_start,
    .analogue_poll = analogue_poll,
    .rotary_init = rotary_init,
    .rotary_count = rotary_count,
};
