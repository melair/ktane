#include "power.h"

#include "fsm.h"
#include "i2c.h"
#include "input_manager.h"
#include "sys/gpio.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define POWER_INIT_DELAY_MS 10U
#define POWER_ENABLE_DELAY_MS 500U
#define POWER_I2C_RETRY_DELAY_MS 100U
#define POWER_CURRENT_SCAN_INTERVAL_MS 100U
#define POWER_CURRENT_AVERAGE_SAMPLE_COUNT 10U

#define POWER_ADC_REFERENCE_VOLTAGE_V 3.3f
#define POWER_ADC_MAX_VALUE 4095.0f
#define POWER_IMON_GAIN_A_PER_A 0.000656f
#define POWER_POT_RESISTANCE_OHMS 10000.0f
#define POWER_POT_POSITION_COUNT 1024.0f
#define POWER_ILM_SERIES_RESISTANCE_OHMS 430.0f

#define POWER_FRONT_POT_I2C_ADDRESS 0x2c
#define POWER_REAR_POT_I2C_ADDRESS 0x2f

#define POWER_POT_UNLOCK_COMMAND 0x1c02
#define POWER_POT_WRITE_RDAC_COMMAND 0x0400
#define POWER_DEFAULT_CURRENT_DECIAMPS 10U

typedef enum {
    POWER_FSM_STATE_INIT = 0,
    POWER_FSM_STATE_UNLOCK_POT,
    POWER_FSM_STATE_IDLE,
    POWER_FSM_STATE_SET_CURRENT,
    POWER_FSM_STATE_ACTIVE,
    POWER_FSM_STATE_TRIP,
    POWER_FSM_STATE_SHUTDOWN,
    POWER_FSM_STATE_COUNT,
} Power_FSM_State;

typedef struct {
    GPIO_PinDef module_detect;
    GPIO_PinDef efuse_enable;
    GPIO_PinDef efuse_fault;
    uint8_t pot_i2c_address;
    IM_AnaloguePinConfig ilm_pin;
} Power_ChannelConfig;

typedef struct {
    FSM fsm;
    const Power_ChannelConfig *config;
    I2C_Transaction transaction;
    uint8_t tx_data[2];
    uint8_t current_limit;
    uint8_t requested_current_limit;
    uint8_t writing_current_limit;
    bool output_active;
    float current_draw_a;
    float current_draw_samples_a[POWER_CURRENT_AVERAGE_SAMPLE_COUNT];
    float current_draw_sum_a;
    uint8_t current_draw_sample_index;
    uint8_t current_draw_sample_count;
    IM_EventQueue current_event_queue;
    IM_AnalogueChannelState current_channel;
    IM_AnalogueInputState current_input_state;
    IM_AnalogueInputConfig current_input;
} Power_Channel;

static const uint16_t power_i2c_ilim_values[] = {
    761U, /* 0.0 A (minimum supported: 0.126 A) */
    761U, /* 0.1 A (minimum supported: 0.126 A) */
    446U, /* 0.2 A */
    276U, /* 0.3 A */
    194U, /* 0.4 A */
    145U, /* 0.5 A */
    113U, /* 0.6 A */
    90U,  /* 0.7 A */
    73U,  /* 0.8 A */
    60U,  /* 0.9 A */
    49U,  /* 1.0 A */
    41U,  /* 1.1 A */
    34U,  /* 1.2 A */
    28U,  /* 1.3 A */
    23U,  /* 1.4 A */
    18U,  /* 1.5 A */
    14U,  /* 1.6 A */
    11U,  /* 1.7 A */
    8U,   /* 1.8 A */
    5U,   /* 1.9 A */
    3U,   /* 2.0 A (1.977 A nominal, within RILM specification) */
};

_Static_assert(POWER_DEFAULT_CURRENT_DECIAMPS <
               (sizeof(power_i2c_ilim_values) / sizeof(power_i2c_ilim_values[0])),
               "Default power current is outside the potentiometer lookup table");

static const Power_ChannelConfig power_channel_configs[POWER_CHANNEL_COUNT] = {
    {
        .module_detect = {FRONT_MODULE_DETECT_GPIO_Port, FRONT_MODULE_DETECT_Pin},
        .efuse_enable = {FRONT_EFUSE_EN_GPIO_Port, FRONT_EFUSE_EN_Pin},
        .efuse_fault = {FRONT_EFUSE_FLT_GPIO_Port, FRONT_EFUSE_FLT_Pin},
        .pot_i2c_address = POWER_FRONT_POT_I2C_ADDRESS,
        .ilm_pin = {
            .pin = {FRONT_ILM_GPIO_Port, FRONT_ILM_Pin},
            .adc_channel = ADC_CHANNEL_10,
        },
    },
    {
        .module_detect = {REAR_MODULE_DETECT_GPIO_Port, REAR_MODULE_DETECT_Pin},
        .efuse_enable = {REAR_EFUSE_EN_GPIO_Port, REAR_EFUSE_EN_Pin},
        .efuse_fault = {REAR_EFUSE_FLT_GPIO_Port, REAR_EFUSE_FLT_Pin},
        .pot_i2c_address = POWER_REAR_POT_I2C_ADDRESS,
        .ilm_pin = {
            .pin = {REAR_ILM_GPIO_Port, REAR_ILM_Pin},
            .adc_channel = ADC_CHANNEL_3,
        },
    },
};

static Power_Channel power_channels[POWER_CHANNEL_COUNT] = {0};

static void power_fsm_init_enter(FSM *fsm);
static void power_fsm_unlock_pot_enter(FSM *fsm);
static void power_fsm_idle_service(FSM *fsm);
static void power_fsm_set_current_enter(FSM *fsm);
static void power_fsm_active_enter(FSM *fsm);
static void power_fsm_active_service(FSM *fsm);
static void power_fsm_trip_enter(FSM *fsm);
static void power_fsm_trip_service(FSM *fsm);
static void power_fsm_shutdown_enter(FSM *fsm);

static const FSM_State power_fsm_states[POWER_FSM_STATE_COUNT] = {
    [POWER_FSM_STATE_INIT] = {
        .enter = power_fsm_init_enter,
        .next_mask = FSM_NEXT(POWER_FSM_STATE_UNLOCK_POT),
    },
    [POWER_FSM_STATE_UNLOCK_POT] = {
        .enter = power_fsm_unlock_pot_enter,
        .next_mask = FSM_NEXT(POWER_FSM_STATE_UNLOCK_POT) |
                     FSM_NEXT(POWER_FSM_STATE_IDLE),
    },
    [POWER_FSM_STATE_IDLE] = {
        .service = power_fsm_idle_service,
        .next_mask = FSM_NEXT(POWER_FSM_STATE_SET_CURRENT),
    },
    [POWER_FSM_STATE_SET_CURRENT] = {
        .enter = power_fsm_set_current_enter,
        .next_mask = FSM_NEXT(POWER_FSM_STATE_ACTIVE) |
                     FSM_NEXT(POWER_FSM_STATE_SHUTDOWN),
    },
    [POWER_FSM_STATE_ACTIVE] = {
        .enter = power_fsm_active_enter,
        .service = power_fsm_active_service,
        .next_mask = FSM_NEXT(POWER_FSM_STATE_SET_CURRENT) |
                     FSM_NEXT(POWER_FSM_STATE_TRIP) |
                     FSM_NEXT(POWER_FSM_STATE_SHUTDOWN),
    },
    [POWER_FSM_STATE_TRIP] = {
        .enter = power_fsm_trip_enter,
        .service = power_fsm_trip_service,
        .next_mask = FSM_NEXT(POWER_FSM_STATE_SHUTDOWN),
    },
    [POWER_FSM_STATE_SHUTDOWN] = {
        .enter = power_fsm_shutdown_enter,
        .next_mask = FSM_NEXT(POWER_FSM_STATE_IDLE),
    },
};

static bool power_module_present(const Power_Channel *channel) {
    return HAL_GPIO_ReadPin(channel->config->module_detect.port,
                            channel->config->module_detect.pin) == GPIO_PIN_RESET;
}

static bool power_efuse_faulted(const Power_Channel *channel) {
    return HAL_GPIO_ReadPin(channel->config->efuse_fault.port,
                            channel->config->efuse_fault.pin) == GPIO_PIN_RESET;
}

static void power_efuse_enable(Power_Channel *channel, const bool enabled) {
    channel->output_active = enabled;
    HAL_GPIO_WritePin(channel->config->efuse_enable.port,
                      channel->config->efuse_enable.pin,
                      enabled ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

static float power_current_from_adc(const Power_Channel *channel, const uint16_t adc_value) {
    const uint16_t pot_value = power_i2c_ilim_values[channel->current_limit];
    const float ilm_resistance_ohms = POWER_ILM_SERIES_RESISTANCE_OHMS +
                                      ((float) pot_value * POWER_POT_RESISTANCE_OHMS /
                                       POWER_POT_POSITION_COUNT);
    const float ilm_voltage_v = (float) adc_value * POWER_ADC_REFERENCE_VOLTAGE_V /
                                POWER_ADC_MAX_VALUE;

    return ilm_voltage_v / (POWER_IMON_GAIN_A_PER_A * ilm_resistance_ohms);
}

static void power_current_service(Power_Channel *channel) {
    IM_Event event;

    while (IM_EventQueue_Read(&channel->current_event_queue, &event)) {
        if ((event.event == IM_EVENT_ANALOGUE) && (event.channel == 0U)) {
            const float current_draw_a = power_current_from_adc(channel, event.value);

            if (channel->current_draw_sample_count == POWER_CURRENT_AVERAGE_SAMPLE_COUNT) {
                channel->current_draw_sum_a -=
                    channel->current_draw_samples_a[channel->current_draw_sample_index];
            } else {
                channel->current_draw_sample_count++;
            }

            channel->current_draw_samples_a[channel->current_draw_sample_index] = current_draw_a;
            channel->current_draw_sum_a += current_draw_a;
            channel->current_draw_sample_index =
                (uint8_t) ((channel->current_draw_sample_index + 1U) %
                           POWER_CURRENT_AVERAGE_SAMPLE_COUNT);
            channel->current_draw_a =
                channel->current_draw_sum_a / (float) channel->current_draw_sample_count;
        }
    }
}

static void power_queue_pot_write(Power_Channel *channel, const uint16_t value) {
    channel->tx_data[0] = (uint8_t) (value >> 8U);
    channel->tx_data[1] = (uint8_t) value;
    channel->transaction.tx_size = sizeof(channel->tx_data);
    I2C_Queue(&channel->transaction);
}

static I2C_Transaction *power_i2c_complete(I2C_Transaction *transaction) {
    Power_Channel *channel = transaction->callback_data;

    if (transaction->status != I2C_STATUS_SUCCESS) {
        power_efuse_enable(channel, false);

        if (channel->fsm.current_id == POWER_FSM_STATE_UNLOCK_POT) {
            FSM_TransitionIn(&channel->fsm, POWER_FSM_STATE_UNLOCK_POT,
                             POWER_I2C_RETRY_DELAY_MS);
        } else if (channel->fsm.current_id == POWER_FSM_STATE_SET_CURRENT) {
            FSM_Transition(&channel->fsm, POWER_FSM_STATE_SHUTDOWN);
        }

        return NULL;
    }

    if (channel->fsm.current_id == POWER_FSM_STATE_UNLOCK_POT) {
        FSM_Transition(&channel->fsm, POWER_FSM_STATE_IDLE);
    } else if (channel->fsm.current_id == POWER_FSM_STATE_SET_CURRENT) {
        channel->current_limit = channel->writing_current_limit;
        FSM_Transition(&channel->fsm,
                       power_module_present(channel)
                           ? POWER_FSM_STATE_ACTIVE
                           : POWER_FSM_STATE_SHUTDOWN);
    }

    return NULL;
}

static void power_fsm_init_enter(FSM *fsm) {
    power_efuse_enable(fsm->context, false);
    FSM_TransitionIn(fsm, POWER_FSM_STATE_UNLOCK_POT, POWER_INIT_DELAY_MS);
}

static void power_fsm_unlock_pot_enter(FSM *fsm) {
    power_queue_pot_write(fsm->context, POWER_POT_UNLOCK_COMMAND);
}

static void power_fsm_idle_service(FSM *fsm) {
    Power_Channel *channel = fsm->context;

    if (power_module_present(channel)) {
        channel->requested_current_limit = POWER_DEFAULT_CURRENT_DECIAMPS;
        FSM_TransitionIn(fsm, POWER_FSM_STATE_SET_CURRENT, POWER_ENABLE_DELAY_MS);
    }
}

static void power_fsm_set_current_enter(FSM *fsm) {
    Power_Channel *channel = fsm->context;

    channel->writing_current_limit = channel->requested_current_limit;
    const uint16_t value = POWER_POT_WRITE_RDAC_COMMAND |
                           power_i2c_ilim_values[channel->writing_current_limit];

    power_queue_pot_write(channel, value);
}

static void power_fsm_active_enter(FSM *fsm) {
    Power_Channel *channel = fsm->context;

    channel->current_draw_a = 0.0f;
    channel->current_draw_sum_a = 0.0f;
    channel->current_draw_sample_index = 0U;
    channel->current_draw_sample_count = 0U;
    IM_EventQueue_Clear(&channel->current_event_queue);
    power_efuse_enable(channel, true);
}

static void power_fsm_active_service(FSM *fsm) {
    Power_Channel *channel = fsm->context;

    power_current_service(channel);

    if (power_efuse_faulted(channel)) {
        FSM_Transition(fsm, POWER_FSM_STATE_TRIP);
    } else if (!power_module_present(channel)) {
        FSM_Transition(fsm, POWER_FSM_STATE_SHUTDOWN);
    } else if (channel->requested_current_limit != channel->current_limit) {
        FSM_Transition(fsm, POWER_FSM_STATE_SET_CURRENT);
    }
}

static void power_fsm_trip_enter(FSM *fsm) {
    power_efuse_enable(fsm->context, false);
}

static void power_fsm_trip_service(FSM *fsm) {
    if (!power_module_present(fsm->context)) {
        FSM_Transition(fsm, POWER_FSM_STATE_SHUTDOWN);
    }
}

static void power_fsm_shutdown_enter(FSM *fsm) {
    power_efuse_enable(fsm->context, false);
    FSM_Transition(fsm, POWER_FSM_STATE_IDLE);
}

static void power_channel_gpio_init(const Power_ChannelConfig *config) {
    GPIO_InitTypeDef gpio_init = {0};

    gpio_init.Pin = config->module_detect.pin;
    gpio_init.Mode = GPIO_MODE_INPUT;
    gpio_init.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(config->module_detect.port, &gpio_init);

    gpio_init.Pin = config->efuse_fault.pin;
    gpio_init.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(config->efuse_fault.port, &gpio_init);

    /* The eFuse enable is active low and open drain. Preload it disabled. */
    HAL_GPIO_WritePin(config->efuse_enable.port, config->efuse_enable.pin, GPIO_PIN_SET);
    gpio_init.Pin = config->efuse_enable.pin;
    gpio_init.Mode = GPIO_MODE_OUTPUT_OD;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(config->efuse_enable.port, &gpio_init);
}

bool Power_Init(void) {
    for (size_t i = 0; i < POWER_CHANNEL_COUNT; i++) {
        Power_Channel *channel = &power_channels[i];
        channel->config = &power_channel_configs[i];
        channel->transaction = (I2C_Transaction) {
            .operation = I2C_OPERATION_WRITE,
            .address = channel->config->pot_i2c_address,
            .tx_data = channel->tx_data,
            .callback = power_i2c_complete,
            .callback_data = channel,
        };
        channel->current_input_state.channels = &channel->current_channel;
        channel->current_input = (IM_AnalogueInputConfig) {
            .pins = &channel->config->ilm_pin,
            .pin_count = 1U,
            .queue = &channel->current_event_queue,
            .state = &channel->current_input_state,
            .scan_interval_ms = POWER_CURRENT_SCAN_INTERVAL_MS,
            .change_threshold = 0U,
        };

        power_channel_gpio_init(channel->config);

        if (!FSM_Init(&channel->fsm, power_fsm_states, POWER_FSM_STATE_INIT, channel)) {
            return false;
        }
        if (IM_RegisterAnalogue(&channel->current_input) == IM_INVALID_HANDLE) {
            return false;
        }
    }

    return true;
}

void Power_Service(void) {
    for (size_t i = 0; i < POWER_CHANNEL_COUNT; i++) {
        FSM_Service(&power_channels[i].fsm);
    }
}

bool Power_SetCurrent(const Power_ChannelId channel_id, const uint8_t current_deciamps) {
    if (((unsigned int) channel_id >= POWER_CHANNEL_COUNT) ||
        (current_deciamps >= (sizeof(power_i2c_ilim_values) / sizeof(power_i2c_ilim_values[0])))) {
        return false;
    }

    Power_Channel *channel = &power_channels[channel_id];
    if (!channel->output_active) {
        return false;
    }

    channel->requested_current_limit = current_deciamps;
    return true;
}
