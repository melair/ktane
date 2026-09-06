#include "input_manager/input_manager_platform.h"

static ADC_HandleTypeDef adc;

static bool analogue_init(void) {
    __HAL_RCC_ADC_CLK_ENABLE();

    adc.Instance = ADC1;
    adc.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
    adc.Init.Resolution = ADC_RESOLUTION_12B;
    adc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    adc.Init.ScanConvMode = ADC_SCAN_DISABLE;
    adc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    adc.Init.LowPowerAutoWait = DISABLE;
    adc.Init.LowPowerAutoPowerOff = DISABLE;
    adc.Init.ContinuousConvMode = DISABLE;
    adc.Init.NbrOfConversion = 1;
    adc.Init.DiscontinuousConvMode = DISABLE;
    adc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    adc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    adc.Init.DMAContinuousRequests = DISABLE;
    adc.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
    adc.Init.SamplingTimeCommon1 = ADC_SAMPLETIME_39CYCLES_5;
    adc.Init.SamplingTimeCommon2 = ADC_SAMPLETIME_39CYCLES_5;
    adc.Init.OversamplingMode = DISABLE;
    adc.Init.TriggerFrequencyMode = ADC_TRIGGER_FREQ_HIGH;

    if (HAL_ADC_Init(&adc) != HAL_OK) {
        return false;
    }

    return HAL_ADCEx_Calibration_Start(&adc) == HAL_OK;
}

static bool analogue_start(uint32_t adc_channel) {
    ADC_ChannelConfTypeDef channel_config = {0};

    channel_config.Channel = adc_channel;
    channel_config.Rank = ADC_REGULAR_RANK_1;
    channel_config.SamplingTime = ADC_SAMPLINGTIME_COMMON_1;

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

const IM_Platform IM_PLATFORM = {
    .analogue_init = analogue_init,
    .analogue_start = analogue_start,
    .analogue_poll = analogue_poll,
    .rotary_init = NULL,
    .rotary_count = NULL,
};
