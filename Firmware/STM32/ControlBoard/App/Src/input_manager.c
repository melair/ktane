#include "input_manager.h"

#define IM_MAX_REGISTRATIONS 8

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

static IM_Registration registrations[IM_MAX_REGISTRATIONS];

static IM_Handle allocate_registration(IM_RegistrationType type) {
    for (IM_Handle handle = 0; handle < IM_MAX_REGISTRATIONS; handle++) {
        if (registrations[handle].type == IM_REGISTRATION_UNUSED) {
            registrations[handle].type = type;
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

static uint8_t digital_channel_index(const IM_DigitalInputConfig *config, uint8_t row, uint8_t col) {
    if (config->col_count == 0) {
        return row;
    }

    return (uint8_t) ((col * config->row_count) + row);
}

static void digital_event_publish(IM_Handle handle, const IM_DigitalInputConfig *config, uint8_t row, uint8_t col,
                                  IM_EventType event_type, uint32_t now_ms, uint32_t duration_ms) {
    if ((config->event_mask & event_type) == 0) {
        return;
    }

    const IM_Event event = {
        .handle = handle,
        .row = row,
        .col = col,
        .event = event_type,
        .timestamp_ms = now_ms,
        .duration_ms = duration_ms,
        .delta = 0,
        .value = 0,
    };

    queue_event(config->queue, &event);
}

static void digital_channel_update(IM_Handle handle, const IM_DigitalInputConfig *config,
                                   IM_DigitalChannelState *channel, uint8_t row, uint8_t col, bool read_down,
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
            digital_event_publish(handle, config, row, col, IM_EVENT_DOWN, now_ms, 0);
        } else {
            digital_event_publish(handle, config, row, col, IM_EVENT_UP, now_ms,
                                  (uint32_t) (now_ms - channel->down_since_ms));
        }

        return;
    }

    if (channel->stable_down && channel->last_read_down && (config->held_event_interval_ms > 0) &&
        has_reached(now_ms, channel->next_held_event_ms)) {
        digital_event_publish(handle, config, row, col, IM_EVENT_HELD, now_ms,
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
        const bool read_down = HAL_GPIO_ReadPin(config->rows[row].port, config->rows[row].pin) == GPIO_PIN_RESET;
        digital_channel_update(handle, config, &state->channels[digital_channel_index(config, row, IM_NO_COLUMN)], row,
                           IM_NO_COLUMN, read_down, now_ms);
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
        const bool read_down = HAL_GPIO_ReadPin(config->rows[row].port, config->rows[row].pin) == GPIO_PIN_RESET;
        digital_channel_update(handle, config, &state->channels[digital_channel_index(config, row, state->active_col)], row,
                           state->active_col, read_down, now_ms);
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
        registrations[handle].type = IM_REGISTRATION_UNUSED;
    }
}

void IM_Service(void) {
    const uint32_t now_ms = HAL_GetTick();

    for (IM_Handle handle = 0; handle < IM_MAX_REGISTRATIONS; handle++) {
        if (registrations[handle].type == IM_REGISTRATION_DIGITAL) {
            digital_service(handle, registrations[handle].config.digital, now_ms);
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
        registrations[handle].config.digital = config;
        *config->state = (IM_DigitalInputState){
            .channels = config->state->channels,
            .channel_count = config->state->channel_count,
            .next_scan_ms = HAL_GetTick(),
        };

        for (uint8_t channel = 0; channel < config->state->channel_count; channel++) {
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
        registrations[handle].config.rotary = config;
    }

    return handle;
}

IM_Handle IM_RegisterAnalogue(const IM_AnalogueInputConfig *config) {
    const IM_Handle handle = allocate_registration(IM_REGISTRATION_ANALOGUE);

    if (handle != IM_INVALID_HANDLE) {
        registrations[handle].config.analogue = config;
    }

    return handle;
}
