#include "sys/input_manager.h"

#define IM_MAX_REGISTRATIONS 8
#define IM_ROTARY_COUNTER_PERIOD 0xFFFFu

typedef struct {
    GPIO_TypeDef *port;
    uint16_t pin;
    uint32_t channel;
} IM_AnaloguePinMap;

typedef enum {
    IM_REGISTRATION_UNUSED,
    IM_REGISTRATION_DIGITAL,
    IM_REGISTRATION_ROTARY,
    IM_REGISTRATION_ANALOGUE,
} IM_RegistrationType;

typedef struct {
    IM_RegistrationType type;

    union {
        const IM_DigitalInputConfig *digital;
        const IM_RotaryEncoderConfig *rotary;
        const IM_AnalogueInputConfig *analogue;
    } config;
} IM_Registration;

typedef struct {
    ADC_HandleTypeDef adc;
    const IM_AnalogueInputConfig *active_config;
    IM_Handle active_handle;
    uint8_t active_pin;
    const IM_AnalogueInputConfig *scan_config;
    IM_Handle scan_handle;
} analogue_t;

typedef struct {
    IM_Registration registrations[IM_MAX_REGISTRATIONS];
    analogue_t analogue;
} im_t;

static im_t im;

static const IM_AnaloguePinMap analogue_pin_map[] = {
    {GPIO_B0_Port, GPIO_B0_Pin, ADC_CHANNEL_18},
    {GPIO_B1_Port, GPIO_B1_Pin, ADC_CHANNEL_19},
    {GPIO_B2_Port, GPIO_B2_Pin, ADC_CHANNEL_3},
    {GPIO_B3_Port, GPIO_B3_Pin, ADC_CHANNEL_7},
    {GPIO_B4_Port, GPIO_B4_Pin, ADC_CHANNEL_4},
    {GPIO_B5_Port, GPIO_B5_Pin, ADC_CHANNEL_8},
    {GPIO_B6_Port, GPIO_B6_Pin, ADC_CHANNEL_9},
    {GPIO_B7_Port, GPIO_B7_Pin, ADC_CHANNEL_5},
    {GPIO_C0_Port, GPIO_C0_Pin, ADC_CHANNEL_0},
    {GPIO_C1_Port, GPIO_C1_Pin, ADC_CHANNEL_1},
    {GPIO_C2_Port, GPIO_C2_Pin, ADC_CHANNEL_14},
    {GPIO_C3_Port, GPIO_C3_Pin, ADC_CHANNEL_15},
    {GPIO_C4_Port, GPIO_C4_Pin, ADC_CHANNEL_10},
    {GPIO_C5_Port, GPIO_C5_Pin, ADC_CHANNEL_11},
    {GPIO_C6_Port, GPIO_C6_Pin, ADC_CHANNEL_12},
    {GPIO_C7_Port, GPIO_C7_Pin, ADC_CHANNEL_13},
};

static IM_Handle allocate_registration(IM_RegistrationType type) {
    for (IM_Handle handle = 0; handle < IM_MAX_REGISTRATIONS; handle++) {
        if (im.registrations[handle].type == IM_REGISTRATION_UNUSED) {
            im.registrations[handle].type = type;
            return handle;
        }
    }

    return IM_INVALID_HANDLE;
}

static void queue_event(IM_EventQueue *queue, const IM_Event *event) {
    if (queue == NULL) {
        return;
    }

    if (queue->count >= IM_QUEUE_SIZE) {
        queue->dropped++;
        return;
    }

    queue->events[queue->write] = *event;

    queue->write++;
    if (queue->write >= IM_QUEUE_SIZE) {
        queue->write = 0;
    }

    queue->count++;
}

static bool has_elapsed(uint32_t now_ms, uint32_t then_ms, uint32_t interval_ms) {
    return now_ms - then_ms >= interval_ms;
}

static bool has_reached(uint32_t now_ms, uint32_t target_ms) {
    return (int32_t) (now_ms - target_ms) >= 0;
}

static bool analogue_channel_get(const GPIO_PinDef *pin, uint32_t *channel) {
    for (uint8_t index = 0; index < (sizeof(analogue_pin_map) / sizeof(analogue_pin_map[0])); index++) {
        if ((analogue_pin_map[index].port == pin->port) && (analogue_pin_map[index].pin == pin->pin)) {
            *channel = analogue_pin_map[index].channel;
            return true;
        }
    }

    return false;
}

static bool analogue_adc_init(void) {
    im.analogue.adc.Instance = ADC1;
    im.analogue.adc.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
    im.analogue.adc.Init.Resolution = ADC_RESOLUTION_12B;
    im.analogue.adc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    im.analogue.adc.Init.ScanConvMode = ADC_SCAN_DISABLE;
    im.analogue.adc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    im.analogue.adc.Init.LowPowerAutoWait = DISABLE;
    im.analogue.adc.Init.ContinuousConvMode = DISABLE;
    im.analogue.adc.Init.NbrOfConversion = 1;
    im.analogue.adc.Init.DiscontinuousConvMode = DISABLE;
    im.analogue.adc.Init.NbrOfDiscConversion = 1;
    im.analogue.adc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    im.analogue.adc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    im.analogue.adc.Init.SamplingMode = ADC_SAMPLING_MODE_NORMAL;
    im.analogue.adc.Init.DMAContinuousRequests = DISABLE;
    im.analogue.adc.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
    im.analogue.adc.Init.OversamplingMode = DISABLE;

    if (HAL_ADC_Init(&im.analogue.adc) != HAL_OK) {
        return false;
    }

    if (HAL_ADCEx_Calibration_Start(&im.analogue.adc, ADC_SINGLE_ENDED) != HAL_OK) {
        return false;
    }

    return true;
}

static bool analogue_conversion_start(IM_Handle handle, const IM_AnalogueInputConfig *config, uint8_t pin_index) {
    uint32_t channel = 0;
    ADC_ChannelConfTypeDef channel_config = {0};

    if (!analogue_channel_get(&config->pins[pin_index], &channel)) {
        return false;
    }

    channel_config.Channel = channel;
    channel_config.Rank = ADC_REGULAR_RANK_1;
    channel_config.SamplingTime = ADC_SAMPLETIME_47CYCLES_5;
    channel_config.SingleDiff = ADC_SINGLE_ENDED;
    channel_config.OffsetNumber = ADC_OFFSET_NONE;
    channel_config.Offset = 0;

    if (HAL_ADC_ConfigChannel(&im.analogue.adc, &channel_config) != HAL_OK) {
        return false;
    }

    if (HAL_ADC_Start(&im.analogue.adc) != HAL_OK) {
        return false;
    }

    im.analogue.active_config = config;
    im.analogue.active_handle = handle;
    im.analogue.active_pin = pin_index;

    return true;
}

static void analogue_value_publish(IM_Handle handle, const IM_AnalogueInputConfig *config, uint8_t pin_index,
                                   uint16_t value, int16_t delta, uint32_t now_ms) {
    const IM_Event event = {
        .handle = handle,
        .channel = pin_index,
        .event = IM_EVENT_ANALOGUE,
        .timestamp_ms = now_ms,
        .duration_ms = 0,
        .delta = delta,
        .value = value,
    };

    queue_event(config->queue, &event);
}

static void analogue_scan_complete(const IM_AnalogueInputConfig *config, uint32_t now_ms) {
    IM_AnalogueInputState *state = config->state;

    state->active_pin = 0;
    state->scan_active = false;
    state->next_scan_ms = now_ms + config->scan_interval_ms;

    if (im.analogue.scan_config == config) {
        im.analogue.scan_config = NULL;
        im.analogue.scan_handle = IM_INVALID_HANDLE;
    }
}

static void analogue_next_pin(const IM_AnalogueInputConfig *config, uint32_t now_ms) {
    IM_AnalogueInputState *state = config->state;

    state->active_pin++;
    if (state->active_pin >= config->pin_count) {
        analogue_scan_complete(config, now_ms);
    }
}

static void analogue_conversion_poll(uint32_t now_ms) {
    if (HAL_ADC_PollForConversion(&im.analogue.adc, 0) != HAL_OK) {
        return;
    }

    const IM_AnalogueInputConfig *config = im.analogue.active_config;
    IM_AnalogueInputState *state = config->state;
    IM_AnalogueChannelState *channel = &state->channels[im.analogue.active_pin];
    const uint16_t value = (uint16_t) HAL_ADC_GetValue(&im.analogue.adc);
    int16_t delta = 0;
    bool publish = config->change_threshold == 0;

    HAL_ADC_Stop(&im.analogue.adc);

    channel->value = value;

    if (!channel->baseline_valid) {
        channel->baseline_valid = true;
        publish = true;
    } else {
        delta = (int16_t) ((int32_t) value - (int32_t) channel->last_published_value);
        publish = publish || ((uint16_t) (delta < 0 ? -delta : delta) >= config->change_threshold);
    }

    if (publish) {
        analogue_value_publish(im.analogue.active_handle, config, im.analogue.active_pin, value, delta, now_ms);
        channel->last_published_value = value;
    }

    analogue_next_pin(config, now_ms);

    im.analogue.active_config = NULL;
    im.analogue.active_handle = IM_INVALID_HANDLE;
}

static void analogue_conversion_start_for_scan(IM_Handle handle, const IM_AnalogueInputConfig *config,
                                               uint32_t now_ms) {
    IM_AnalogueInputState *state = config->state;

    if (!analogue_conversion_start(handle, config, state->active_pin)) {
        analogue_next_pin(config, now_ms);
    }
}

static void analogue_conversion_start_due(uint32_t now_ms) {
    if (im.analogue.scan_config != NULL) {
        analogue_conversion_start_for_scan(im.analogue.scan_handle, im.analogue.scan_config, now_ms);
        return;
    }

    for (IM_Handle handle = 0; handle < IM_MAX_REGISTRATIONS; handle++) {
        if (im.registrations[handle].type != IM_REGISTRATION_ANALOGUE) {
            continue;
        }

        const IM_AnalogueInputConfig *config = im.registrations[handle].config.analogue;
        IM_AnalogueInputState *state = config->state;

        if (!state->scan_active) {
            if (!has_reached(now_ms, state->next_scan_ms)) {
                continue;
            }

            state->active_pin = 0;
            state->scan_active = true;
            im.analogue.scan_config = config;
            im.analogue.scan_handle = handle;
        }

        analogue_conversion_start_for_scan(handle, config, now_ms);
        return;
    }
}

static void analogue_service(uint32_t now_ms) {
    if (im.analogue.active_config == NULL) {
        analogue_conversion_start_due(now_ms);
    } else {
        analogue_conversion_poll(now_ms);
    }
}

static void analogue_pin_configure(const GPIO_PinDef *pin) {
    GPIO_InitTypeDef gpio_init = {0};

    gpio_init.Pin = pin->pin;
    gpio_init.Mode = GPIO_MODE_ANALOG;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(pin->port, &gpio_init);
}

static TIM_TypeDef *rotary_timer_instance(IM_RotarySlot slot) {
    switch (slot) {
        case IM_ROTARY_SLOT_MODULE_A:
            return TIM1;
        case IM_ROTARY_SLOT_MODULE_B:
            return TIM3;
        case IM_ROTARY_SLOT_MODULE_C:
            return TIM5;
    }

    return TIM1;
}

static const IM_RotaryEncoderConfig *rotary_config_get(IM_RotarySlot slot) {
    for (IM_Handle handle = 0; handle < IM_MAX_REGISTRATIONS; handle++) {
        if ((im.registrations[handle].type == IM_REGISTRATION_ROTARY) && (
                im.registrations[handle].config.rotary->slot == slot)) {
            return im.registrations[handle].config.rotary;
        }
    }

    return NULL;
}

static void rotary_delta_publish(const IM_RotaryEncoderConfig *config, int16_t delta, uint32_t now_ms) {
    if (config->invert_direction) {
        delta = (int16_t) -delta;
    }

    const IM_Event event = {
        .handle = config->state->input_handle,
        .channel = 0,
        .event = IM_EVENT_ROTARY_DELTA,
        .timestamp_ms = now_ms,
        .duration_ms = 0,
        .delta = delta,
        .value = 0,
    };

    queue_event(config->queue, &event);
}

static void rotary_service(const IM_RotaryEncoderConfig *config, uint32_t now_ms) {
    IM_RotaryEncoderState *state = config->state;
    const uint16_t current_count = (uint16_t) __HAL_TIM_GET_COUNTER(&state->handle);
    const int16_t raw_delta = (int16_t) (current_count - state->last_count);

    state->last_count = current_count;

    if (raw_delta != 0) {
        const int16_t counts_per_detent = (int16_t) config->counts_per_detent;
        if (counts_per_detent <= 0) {
            return;
        }

        state->residual_counts = (int16_t) (state->residual_counts + raw_delta);

        const int16_t detents = (int16_t) (state->residual_counts / counts_per_detent);

        if (detents != 0) {
            rotary_delta_publish(config, detents, now_ms);
            state->residual_counts = (int16_t) (state->residual_counts - (detents * counts_per_detent));
        }
    }
}

static bool rotary_timer_configure(const IM_RotaryEncoderConfig *config) {
    IM_RotaryEncoderState *state = config->state;
    TIM_Encoder_InitTypeDef encoder_config = {0};
    TIM_MasterConfigTypeDef master_config = {0};

    state->handle.Instance = rotary_timer_instance(config->slot);
    state->handle.Init.Prescaler = 0;
    state->handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    state->handle.Init.Period = IM_ROTARY_COUNTER_PERIOD;
    state->handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    state->handle.Init.RepetitionCounter = 0;
    state->handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    encoder_config.EncoderMode = TIM_ENCODERMODE_TI12;
    encoder_config.IC1Polarity = TIM_ICPOLARITY_RISING;
    encoder_config.IC1Selection = TIM_ICSELECTION_DIRECTTI;
    encoder_config.IC1Prescaler = TIM_ICPSC_DIV1;
    encoder_config.IC1Filter = 15;
    encoder_config.IC2Polarity = TIM_ICPOLARITY_RISING;
    encoder_config.IC2Selection = TIM_ICSELECTION_DIRECTTI;
    encoder_config.IC2Prescaler = TIM_ICPSC_DIV1;
    encoder_config.IC2Filter = 15;

    if (HAL_TIM_Encoder_Init(&state->handle, &encoder_config) != HAL_OK) {
        return false;
    }

    master_config.MasterOutputTrigger = TIM_TRGO_RESET;
    master_config.MasterOutputTrigger2 = TIM_TRGO2_RESET;
    master_config.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&state->handle, &master_config) != HAL_OK) {
        return false;
    }

    __HAL_TIM_SET_COUNTER(&state->handle, 0);
    state->last_count = 0;

    return HAL_TIM_Encoder_Start(&state->handle, TIM_CHANNEL_ALL) == HAL_OK;
}

static uint8_t digital_channel_index(const IM_DigitalInputConfig *config, uint8_t row, uint8_t col) {
    if (config->col_count == 0) {
        return row;
    }

    return (uint8_t) ((col * config->row_count) + row);
}

static uint8_t digital_channel_count(const IM_DigitalInputConfig *config) {
    if (config->col_count == 0) {
        return config->row_count;
    }

    return (uint8_t) (config->row_count * config->col_count);
}

static void digital_event_publish(IM_Handle handle, const IM_DigitalInputConfig *config, uint8_t channel,
                                  IM_EventType event_type, uint32_t now_ms, uint32_t duration_ms) {
    if ((config->event_mask & event_type) == 0) {
        return;
    }

    const IM_Event event = {
        .handle = handle,
        .channel = channel,
        .event = event_type,
        .timestamp_ms = now_ms,
        .duration_ms = duration_ms,
        .delta = 0,
        .value = 0,
    };

    queue_event(config->queue, &event);
}

static void digital_channel_update(IM_Handle handle, const IM_DigitalInputConfig *config,
                                   IM_DigitalChannelState *channel, uint8_t event_channel, bool read_down,
                                   uint32_t now_ms) {
    if (!channel->initialized) {
        channel->initialized = true;
        channel->stable_down = read_down;
        channel->last_read_down = read_down;
        channel->last_read_change_ms = now_ms;

        if (read_down) {
            channel->down_since_ms = now_ms;
            channel->next_held_event_ms = now_ms + config->held_event_interval_ms;
        }

        return;
    }

    if (channel->last_read_down != read_down) {
        channel->last_read_down = read_down;
        channel->last_read_change_ms = now_ms;
        return;
    }

    if (channel->stable_down != read_down) {
        if (!has_elapsed(now_ms, channel->last_read_change_ms, config->debounce_ms)) {
            return;
        }

        channel->stable_down = read_down;

        if (read_down) {
            channel->down_since_ms = now_ms;
            channel->next_held_event_ms = now_ms + config->held_event_interval_ms;
            digital_event_publish(handle, config, event_channel, IM_EVENT_DOWN, now_ms, 0);
        } else {
            digital_event_publish(handle, config, event_channel, IM_EVENT_UP, now_ms,
                                  (uint32_t) (now_ms - channel->down_since_ms));
        }

        return;
    }

    if (channel->stable_down && channel->last_read_down && (config->held_event_interval_ms > 0) &&
        has_reached(now_ms, channel->next_held_event_ms)) {
        digital_event_publish(handle, config, event_channel, IM_EVENT_HELD, now_ms,
                              (uint32_t) (now_ms - channel->down_since_ms));
        channel->next_held_event_ms = now_ms + config->held_event_interval_ms;
    }
}

static void digital_input_pin_configure(const GPIO_PinDef *pin, bool enable_internal_pullup) {
    GPIO_InitTypeDef gpio_init = {0};

    gpio_init.Pin = pin->pin;
    gpio_init.Mode = GPIO_MODE_INPUT;
    gpio_init.Pull = enable_internal_pullup ? GPIO_PULLUP : GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(pin->port, &gpio_init);
}

static void digital_column_pin_configure(const GPIO_PinDef *pin) {
    GPIO_InitTypeDef gpio_init = {0};

    HAL_GPIO_WritePin(pin->port, pin->pin, GPIO_PIN_SET);

    gpio_init.Pin = pin->pin;
    gpio_init.Mode = GPIO_MODE_OUTPUT_OD;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(pin->port, &gpio_init);
}

static void digital_columns_release(const IM_DigitalInputConfig *config) {
    for (uint8_t col = 0; col < config->col_count; col++) {
        HAL_GPIO_WritePin(config->cols[col].port, config->cols[col].pin, GPIO_PIN_SET);
    }
}

static bool digital_input_is_down(const IM_DigitalInputConfig *config, const GPIO_PinDef *pin) {
    const GPIO_PinState pin_state = HAL_GPIO_ReadPin(pin->port, pin->pin);

    return config->active_high ? pin_state == GPIO_PIN_SET : pin_state == GPIO_PIN_RESET;
}

static void digital_column_strobe(const IM_DigitalInputConfig *config, uint8_t active_col) {
    digital_columns_release(config);
    HAL_GPIO_WritePin(config->cols[active_col].port, config->cols[active_col].pin, GPIO_PIN_RESET);
}

static void digital_direct_service(IM_Handle handle, const IM_DigitalInputConfig *config, uint32_t now_ms) {
    IM_DigitalInputState *state = config->state;

    if (!has_reached(now_ms, state->next_scan_ms)) {
        return;
    }

    state->next_scan_ms = now_ms + config->scan_period_ms;

    for (uint8_t row = 0; row < config->row_count; row++) {
        const bool read_down = digital_input_is_down(config, &config->rows[row]);
        const uint8_t channel = digital_channel_index(config, row, IM_NO_COLUMN);
        digital_channel_update(handle, config, &state->channels[channel], channel, read_down, now_ms);
    }
}

static void digital_matrix_service(IM_Handle handle, const IM_DigitalInputConfig *config, uint32_t now_ms) {
    IM_DigitalInputState *state = config->state;

    if (!state->column_strobed) {
        if (!has_reached(now_ms, state->next_scan_ms)) {
            return;
        }

        state->active_col = 0;
        digital_column_strobe(config, state->active_col);
        state->column_strobed = true;
        return;
    }

    for (uint8_t row = 0; row < config->row_count; row++) {
        const bool read_down = digital_input_is_down(config, &config->rows[row]);
        const uint8_t channel = digital_channel_index(config, row, state->active_col);
        digital_channel_update(handle, config, &state->channels[channel], channel, read_down, now_ms);
    }

    if ((state->active_col + 1) >= config->col_count) {
        digital_columns_release(config);
        state->column_strobed = false;
        state->next_scan_ms = now_ms + config->scan_period_ms;
        return;
    }

    state->active_col++;
    digital_column_strobe(config, state->active_col);
}

static void digital_service(IM_Handle handle, const IM_DigitalInputConfig *config, uint32_t now_ms) {
    if (config->col_count == 0) {
        digital_direct_service(handle, config, now_ms);
    } else {
        digital_matrix_service(handle, config, now_ms);
    }
}

void IM_Init(void) {
    for (IM_Handle handle = 0; handle < IM_MAX_REGISTRATIONS; handle++) {
        im.registrations[handle].type = IM_REGISTRATION_UNUSED;
    }

    im.analogue.active_config = NULL;
    im.analogue.active_handle = IM_INVALID_HANDLE;
    im.analogue.scan_config = NULL;
    im.analogue.scan_handle = IM_INVALID_HANDLE;
    analogue_adc_init();
}

void IM_Service(void) {
    const uint32_t now_ms = HAL_GetTick();

    analogue_service(now_ms);

    for (IM_Handle handle = 0; handle < IM_MAX_REGISTRATIONS; handle++) {
        switch (im.registrations[handle].type) {
            case IM_REGISTRATION_DIGITAL:
                digital_service(handle, im.registrations[handle].config.digital, now_ms);
                break;
            case IM_REGISTRATION_ROTARY:
                rotary_service(im.registrations[handle].config.rotary, now_ms);
                break;
            default:
                break;
        }
    }
}

bool IM_EventQueue_Read(IM_EventQueue *queue, IM_Event *event) {
    if (queue->count == 0) {
        return false;
    }

    *event = queue->events[queue->read];

    queue->read++;
    if (queue->read >= IM_QUEUE_SIZE) {
        queue->read = 0;
    }

    queue->count--;

    return true;
}

void IM_EventQueue_Clear(IM_EventQueue *queue) {
    queue->read = 0;
    queue->write = 0;
    queue->count = 0;
    queue->dropped = 0;
}

bool IM_DigitalChannel_Get(const IM_DigitalInputConfig *config, uint8_t row, uint8_t col) {
    const IM_DigitalChannelState *channel = &config->state->channels[digital_channel_index(config, row, col)];

    return channel->stable_down;
}

IM_Handle IM_RegisterDigital(const IM_DigitalInputConfig *config) {
    const IM_Handle handle = allocate_registration(IM_REGISTRATION_DIGITAL);

    if (handle != IM_INVALID_HANDLE) {
        im.registrations[handle].config.digital = config;
        *config->state = (IM_DigitalInputState){
            .channels = config->state->channels,
            .next_scan_ms = HAL_GetTick(),
        };

        for (uint8_t channel = 0; channel < digital_channel_count(config); channel++) {
            config->state->channels[channel] = (IM_DigitalChannelState){0};
        }

        for (uint8_t row = 0; row < config->row_count; row++) {
            digital_input_pin_configure(&config->rows[row], config->enable_internal_pullups);
        }

        for (uint8_t col = 0; col < config->col_count; col++) {
            digital_column_pin_configure(&config->cols[col]);
        }
    }

    return handle;
}

IM_Handle IM_RegisterRotaryEncoder(const IM_RotaryEncoderConfig *config) {
    const IM_Handle handle = allocate_registration(IM_REGISTRATION_ROTARY);

    if (handle != IM_INVALID_HANDLE) {
        im.registrations[handle].config.rotary = config;
        *config->state = (IM_RotaryEncoderState){
            .input_handle = handle,
        };

        if (!rotary_timer_configure(config)) {
            im.registrations[handle].type = IM_REGISTRATION_UNUSED;
            return IM_INVALID_HANDLE;
        }
    }

    return handle;
}

IM_Handle IM_RegisterAnalogue(const IM_AnalogueInputConfig *config) {
    const IM_Handle handle = allocate_registration(IM_REGISTRATION_ANALOGUE);

    if (handle != IM_INVALID_HANDLE) {
        im.registrations[handle].config.analogue = config;

        *config->state = (IM_AnalogueInputState){
            .channels = config->state->channels,
            .next_scan_ms = HAL_GetTick(),
        };

        for (uint8_t channel = 0; channel < config->pin_count; channel++) {
            config->state->channels[channel] = (IM_AnalogueChannelState){0};
        }

        for (uint8_t pin = 0; pin < config->pin_count; pin++) {
            uint32_t adc_channel = 0;
            if (!analogue_channel_get(&config->pins[pin], &adc_channel)) {
                im.registrations[handle].type = IM_REGISTRATION_UNUSED;
                return IM_INVALID_HANDLE;
            }

            analogue_pin_configure(&config->pins[pin]);
        }
    }

    return handle;
}

void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc) {
    if (hadc->Instance == ADC1) {
        __HAL_RCC_ADCDAC_CONFIG(RCC_ADCDACCLKSOURCE_PLL2R);
        __HAL_RCC_ADC_CLK_ENABLE();
    }
}

void HAL_TIM_Encoder_MspInit(TIM_HandleTypeDef *htim) {
    GPIO_InitTypeDef gpio_init = {0};
    const IM_RotaryEncoderConfig *config = NULL;

    if (htim->Instance == TIM1) {
        config = rotary_config_get(IM_ROTARY_SLOT_MODULE_A);
        __HAL_RCC_TIM1_CLK_ENABLE();

        gpio_init.Pin = GPIO_A6_Pin | GPIO_A4_Pin;
        gpio_init.Mode = GPIO_MODE_AF_PP;
        gpio_init.Pull = config->enable_internal_pullups ? GPIO_PULLUP : GPIO_NOPULL;
        gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
        gpio_init.Alternate = GPIO_AF1_TIM1;
        HAL_GPIO_Init(GPIOE, &gpio_init);
    } else if (htim->Instance == TIM3) {
        config = rotary_config_get(IM_ROTARY_SLOT_MODULE_B);
        __HAL_RCC_TIM3_CLK_ENABLE();

        gpio_init.Pin = GPIO_B2_Pin | GPIO_B3_Pin;
        gpio_init.Mode = GPIO_MODE_AF_PP;
        gpio_init.Pull = config->enable_internal_pullups ? GPIO_PULLUP : GPIO_NOPULL;
        gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
        gpio_init.Alternate = GPIO_AF2_TIM3;
        HAL_GPIO_Init(GPIOA, &gpio_init);
    } else if (htim->Instance == TIM5) {
        config = rotary_config_get(IM_ROTARY_SLOT_MODULE_C);
        __HAL_RCC_TIM5_CLK_ENABLE();

        gpio_init.Pin = GPIO_C0_Pin | GPIO_C1_Pin;
        gpio_init.Mode = GPIO_MODE_AF_PP;
        gpio_init.Pull = config->enable_internal_pullups ? GPIO_PULLUP : GPIO_NOPULL;
        gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
        gpio_init.Alternate = GPIO_AF2_TIM5;
        HAL_GPIO_Init(GPIOA, &gpio_init);
    }
}
