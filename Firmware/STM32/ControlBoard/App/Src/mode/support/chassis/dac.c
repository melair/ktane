#include "mode/support/chassis/dac.h"
#include "mode.h"
#include "sys/gpio.h"
#include <string.h>

#define DAC_Powerdown_Pin GPIO_B2_Pin
#define DAC_Powerdown_Port GPIO_B2_Port
#define DAC_Fault_Pin GPIO_B5_Pin
#define DAC_Fault_Port GPIO_B5_Port
#define DAC_PVDD_Pin GPIO_B6_Pin
#define DAC_PVDD_Port GPIO_B6_Port
#define DAC_Reset_Pin GPIO_B7_Pin
#define DAC_Reset_Port GPIO_B7_Port

#define DAC_BOOT_DELAY_MS 10
#define DAC_POWERDOWN_RELEASE_DELAY_MS 20
#define DAC_RESET_RELEASE_DELAY_MS 20
#define DAC_PVDD_POWER_UP_DELAY_MS 5
#define DAC_TRIM_OSCILLATOR_DELAY_MS 50
#define DAC_PWM_SWITCHING_RATE_DELAY_MS 250

#define DAC_I2C_ADDRESS (0x54 >> 1)
#define DAC_VOLUME_REGISTER 0x07
#define DAC_VOLUME_LEVEL_MAX 100u
#define DAC_VOLUME_LEVEL_MIN 1u
#define DAC_VOLUME_LEVEL_INTERVALS (DAC_VOLUME_LEVEL_MAX - DAC_VOLUME_LEVEL_MIN)
#define DAC_VOLUME_0_DB 0x00c0u
#define DAC_VOLUME_INITIAL 0x03fe
#define DAC_VOLUME_MUTE 0x03ff
#define DAC_VOLUME_REGISTER_RANGE (DAC_VOLUME_INITIAL - DAC_VOLUME_0_DB)

typedef enum {
    DAC_FSM_STATE_INIT = 0,
    DAC_FSM_STATE_POWERDOWN_RELEASE,
    DAC_FSM_STATE_RESET_RELEASE,
    DAC_FSM_STATE_PVDD_POWER_UP,
    DAC_FSM_STATE_TRIM_OSCILLATOR,
    DAC_FSM_STATE_TRIM_OSCILLATOR_DELAY,
    DAC_FSM_STATE_CONFIGURE_I2S,
    DAC_FSM_STATE_STEREO_BD_MODE_1,
    DAC_FSM_STATE_STEREO_BD_MODE_2,
    DAC_FSM_STATE_STEREO_BD_MODE_3,
    DAC_FSM_STATE_STEREO_BD_MODE_4,
    DAC_FSM_STATE_INPUT_MUX_CONFIG,
    DAC_FSM_STATE_PWM_SWITCHING_RATE,
    DAC_FSM_STATE_PWM_SWITCHING_RATE_DELAY,
    DAC_FSM_STATE_EXIT_SHUTDOWN,
    DAC_FSM_STATE_RUNNING,
    DAC_FSM_STATE_VOLUME,
    DAC_FSM_STATE_COUNT,
} DAC_FSM_State;

typedef struct {
    const uint8_t *data;
    uint16_t size;
    DAC_FSM_State next_state;
} DAC_I2C_Command;

static DAC_Data *const dac = &module_data.mode.chassis.dac;

static const uint8_t dac_trim_oscillator_data[] = {0x1b, 0x00};
static const uint8_t dac_configure_i2s_data[] = {0x04, 0x03};
static const uint8_t dac_stereo_bd_mode_1_data[] = {0x11, 0xb8};
static const uint8_t dac_stereo_bd_mode_2_data[] = {0x12, 0x60};
static const uint8_t dac_stereo_bd_mode_3_data[] = {0x13, 0xa0};
static const uint8_t dac_stereo_bd_mode_4_data[] = {0x14, 0x48};
static const uint8_t dac_input_mux_config_data[] = {0x20, 0x00, 0x89, 0x77, 0x72};
static const uint8_t dac_pwm_switching_rate_data[] = {0x4f, 0x00, 0x00, 0x00, 0x08};
static const uint8_t dac_exit_shutdown_data[] = {0x05, 0x00};

static const DAC_I2C_Command dac_i2c_commands[DAC_FSM_STATE_COUNT] = {
    [DAC_FSM_STATE_TRIM_OSCILLATOR] = {
        .data = dac_trim_oscillator_data,
        .size = sizeof(dac_trim_oscillator_data),
        .next_state = DAC_FSM_STATE_TRIM_OSCILLATOR_DELAY,
    },
    [DAC_FSM_STATE_CONFIGURE_I2S] = {
        .data = dac_configure_i2s_data,
        .size = sizeof(dac_configure_i2s_data),
        .next_state = DAC_FSM_STATE_STEREO_BD_MODE_1,
    },
    [DAC_FSM_STATE_STEREO_BD_MODE_1] = {
        .data = dac_stereo_bd_mode_1_data,
        .size = sizeof(dac_stereo_bd_mode_1_data),
        .next_state = DAC_FSM_STATE_STEREO_BD_MODE_2,
    },
    [DAC_FSM_STATE_STEREO_BD_MODE_2] = {
        .data = dac_stereo_bd_mode_2_data,
        .size = sizeof(dac_stereo_bd_mode_2_data),
        .next_state = DAC_FSM_STATE_STEREO_BD_MODE_3,
    },
    [DAC_FSM_STATE_STEREO_BD_MODE_3] = {
        .data = dac_stereo_bd_mode_3_data,
        .size = sizeof(dac_stereo_bd_mode_3_data),
        .next_state = DAC_FSM_STATE_STEREO_BD_MODE_4,
    },
    [DAC_FSM_STATE_STEREO_BD_MODE_4] = {
        .data = dac_stereo_bd_mode_4_data,
        .size = sizeof(dac_stereo_bd_mode_4_data),
        .next_state = DAC_FSM_STATE_INPUT_MUX_CONFIG,
    },
    [DAC_FSM_STATE_INPUT_MUX_CONFIG] = {
        .data = dac_input_mux_config_data,
        .size = sizeof(dac_input_mux_config_data),
        .next_state = DAC_FSM_STATE_PWM_SWITCHING_RATE,
    },
    [DAC_FSM_STATE_PWM_SWITCHING_RATE] = {
        .data = dac_pwm_switching_rate_data,
        .size = sizeof(dac_pwm_switching_rate_data),
        .next_state = DAC_FSM_STATE_PWM_SWITCHING_RATE_DELAY,
    },
    [DAC_FSM_STATE_EXIT_SHUTDOWN] = {
        .data = dac_exit_shutdown_data,
        .size = sizeof(dac_exit_shutdown_data),
        .next_state = DAC_FSM_STATE_RUNNING,
    },
};

static uint16_t dac_volume_register(const uint8_t volume) {
    const uint32_t scaled_range =
            (uint32_t) (volume - DAC_VOLUME_LEVEL_MIN) * DAC_VOLUME_REGISTER_RANGE;
    const uint16_t register_offset =
            (uint16_t) ((scaled_range + (DAC_VOLUME_LEVEL_INTERVALS / 2u)) /
                        DAC_VOLUME_LEVEL_INTERVALS);
    return DAC_VOLUME_INITIAL - register_offset;
}

static uint16_t dac_volume_target(void) {
    return (dac->mute_override || (dac->volume == 0u))
               ? DAC_VOLUME_MUTE
               : dac_volume_register(dac->volume);
}

static I2C_Transaction *dac_i2c_complete(I2C_Transaction *transaction) {
    if (transaction->status == I2C_STATUS_SUCCESS) {
        if ((dac->fsm.current_id == DAC_FSM_STATE_VOLUME) &&
            (dac->volume_write != dac_volume_target())) {
            FSM_Transition(&dac->fsm, DAC_FSM_STATE_VOLUME);
        } else {
            FSM_Transition(&dac->fsm, dac->transaction_next_state);
        }
    }

    return NULL;
}

static void dac_fsm_i2c_enter(FSM *fsm) {
    const DAC_I2C_Command *command = &dac_i2c_commands[fsm->current_id];

    memcpy(dac->tx_data, command->data, command->size);
    dac->transaction.tx_size = command->size;
    dac->transaction_next_state = command->next_state;
    I2C_Queue(&dac->transaction);
}

static void dac_fsm_init_enter(FSM *fsm) {
    FSM_TransitionIn(fsm, DAC_FSM_STATE_POWERDOWN_RELEASE, DAC_BOOT_DELAY_MS);
}

static void dac_fsm_powerdown_release_enter(FSM *fsm) {
    HAL_GPIO_WritePin(DAC_Powerdown_Port, DAC_Powerdown_Pin, GPIO_PIN_SET);
    FSM_TransitionIn(fsm, DAC_FSM_STATE_RESET_RELEASE, DAC_POWERDOWN_RELEASE_DELAY_MS);
}

static void dac_fsm_reset_release_enter(FSM *fsm) {
    HAL_GPIO_WritePin(DAC_Reset_Port, DAC_Reset_Pin, GPIO_PIN_SET);
    FSM_TransitionIn(fsm, DAC_FSM_STATE_PVDD_POWER_UP, DAC_RESET_RELEASE_DELAY_MS);
}

static void dac_fsm_pvdd_power_up_enter(FSM *fsm) {
    HAL_GPIO_WritePin(DAC_PVDD_Port, DAC_PVDD_Pin, GPIO_PIN_SET);
    FSM_TransitionIn(fsm, DAC_FSM_STATE_TRIM_OSCILLATOR, DAC_PVDD_POWER_UP_DELAY_MS);
}

static void dac_fsm_trim_oscillator_delay_enter(FSM *fsm) {
    FSM_TransitionIn(fsm, DAC_FSM_STATE_CONFIGURE_I2S, DAC_TRIM_OSCILLATOR_DELAY_MS);
}

static void dac_fsm_pwm_switching_rate_delay_enter(FSM *fsm) {
    FSM_TransitionIn(fsm, DAC_FSM_STATE_EXIT_SHUTDOWN, DAC_PWM_SWITCHING_RATE_DELAY_MS);
}

static void dac_fsm_exit_shutdown_exit(FSM *fsm) {
    dac->ready = true;
}

static void dac_fsm_volume_enter(FSM *fsm) {
    dac->volume_write = dac_volume_target();
    dac->tx_data[0] = DAC_VOLUME_REGISTER;
    dac->tx_data[1] = (uint8_t) (dac->volume_write >> 8u);
    dac->tx_data[2] = (uint8_t) dac->volume_write;
    dac->transaction.tx_size = 3u;
    dac->transaction_next_state = DAC_FSM_STATE_RUNNING;
    I2C_Queue(&dac->transaction);
}

static const FSM_State dac_fsm_states[DAC_FSM_STATE_COUNT] = {
    [DAC_FSM_STATE_INIT] = {
        .enter = dac_fsm_init_enter,
        .next_mask = FSM_NEXT(DAC_FSM_STATE_POWERDOWN_RELEASE),
    },
    [DAC_FSM_STATE_POWERDOWN_RELEASE] = {
        .enter = dac_fsm_powerdown_release_enter,
        .next_mask = FSM_NEXT(DAC_FSM_STATE_RESET_RELEASE),
    },
    [DAC_FSM_STATE_RESET_RELEASE] = {
        .enter = dac_fsm_reset_release_enter,
        .next_mask = FSM_NEXT(DAC_FSM_STATE_PVDD_POWER_UP),
    },
    [DAC_FSM_STATE_PVDD_POWER_UP] = {
        .enter = dac_fsm_pvdd_power_up_enter,
        .next_mask = FSM_NEXT(DAC_FSM_STATE_TRIM_OSCILLATOR),
    },
    [DAC_FSM_STATE_TRIM_OSCILLATOR] = {
        .enter = dac_fsm_i2c_enter,
        .next_mask = FSM_NEXT(DAC_FSM_STATE_TRIM_OSCILLATOR_DELAY),
    },
    [DAC_FSM_STATE_TRIM_OSCILLATOR_DELAY] = {
        .enter = dac_fsm_trim_oscillator_delay_enter,
        .next_mask = FSM_NEXT(DAC_FSM_STATE_CONFIGURE_I2S),
    },
    [DAC_FSM_STATE_CONFIGURE_I2S] = {
        .enter = dac_fsm_i2c_enter,
        .next_mask = FSM_NEXT(DAC_FSM_STATE_STEREO_BD_MODE_1),
    },
    [DAC_FSM_STATE_STEREO_BD_MODE_1] = {
        .enter = dac_fsm_i2c_enter,
        .next_mask = FSM_NEXT(DAC_FSM_STATE_STEREO_BD_MODE_2),
    },
    [DAC_FSM_STATE_STEREO_BD_MODE_2] = {
        .enter = dac_fsm_i2c_enter,
        .next_mask = FSM_NEXT(DAC_FSM_STATE_STEREO_BD_MODE_3),
    },
    [DAC_FSM_STATE_STEREO_BD_MODE_3] = {
        .enter = dac_fsm_i2c_enter,
        .next_mask = FSM_NEXT(DAC_FSM_STATE_STEREO_BD_MODE_4),
    },
    [DAC_FSM_STATE_STEREO_BD_MODE_4] = {
        .enter = dac_fsm_i2c_enter,
        .next_mask = FSM_NEXT(DAC_FSM_STATE_INPUT_MUX_CONFIG),
    },
    [DAC_FSM_STATE_INPUT_MUX_CONFIG] = {
        .enter = dac_fsm_i2c_enter,
        .next_mask = FSM_NEXT(DAC_FSM_STATE_PWM_SWITCHING_RATE),
    },
    [DAC_FSM_STATE_PWM_SWITCHING_RATE] = {
        .enter = dac_fsm_i2c_enter,
        .next_mask = FSM_NEXT(DAC_FSM_STATE_PWM_SWITCHING_RATE_DELAY),
    },
    [DAC_FSM_STATE_PWM_SWITCHING_RATE_DELAY] = {
        .enter = dac_fsm_pwm_switching_rate_delay_enter,
        .next_mask = FSM_NEXT(DAC_FSM_STATE_EXIT_SHUTDOWN),
    },
    [DAC_FSM_STATE_EXIT_SHUTDOWN] = {
        .enter = dac_fsm_i2c_enter,
        .exit = dac_fsm_exit_shutdown_exit,
        .next_mask = FSM_NEXT(DAC_FSM_STATE_RUNNING),
    },
    [DAC_FSM_STATE_RUNNING] = {
        .next_mask = FSM_NEXT(DAC_FSM_STATE_VOLUME),
    },
    [DAC_FSM_STATE_VOLUME] = {
        .enter = dac_fsm_volume_enter,
        .next_mask = FSM_NEXT(DAC_FSM_STATE_VOLUME) |
                     FSM_NEXT(DAC_FSM_STATE_RUNNING),
    },
};

void DAC_Init(void) {
    *dac = (DAC_Data){0};
    dac->volume = DAC_VOLUME_LEVEL_MIN;
    dac->mute_override = true;

    dac->transaction = (I2C_Transaction){
        .operation = I2C_OPERATION_WRITE,
        .address = DAC_I2C_ADDRESS,
        .tx_data = dac->tx_data,
        .callback = dac_i2c_complete,
    };

    GPIO_InitTypeDef gpio_init = {0};

    HAL_GPIO_WritePin(DAC_Powerdown_Port, DAC_Powerdown_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DAC_Fault_Port, DAC_Fault_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DAC_PVDD_Port, DAC_PVDD_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DAC_Reset_Port, DAC_Reset_Pin, GPIO_PIN_RESET);

    gpio_init.Pin = DAC_Powerdown_Pin;
    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(DAC_Powerdown_Port, &gpio_init);

    gpio_init.Pin = DAC_PVDD_Pin;
    HAL_GPIO_Init(DAC_PVDD_Port, &gpio_init);

    gpio_init.Pin = DAC_Reset_Pin;
    HAL_GPIO_Init(DAC_Reset_Port, &gpio_init);

    gpio_init.Pin = DAC_Fault_Pin;
    HAL_GPIO_Init(DAC_Fault_Port, &gpio_init);

    FSM_Init(&dac->fsm, dac_fsm_states, DAC_FSM_STATE_INIT, dac);
}

void DAC_Service(void) {
    FSM_Service(&dac->fsm);
}

bool DAC_Ready(void) {
    return dac->ready;
}

void DAC_Volume(uint8_t volume) {
    if (volume > DAC_VOLUME_LEVEL_MAX) {
        volume = DAC_VOLUME_LEVEL_MAX;
    }

    const uint16_t previous_target = dac_volume_target();
    dac->volume = volume;

    if ((dac_volume_target() != previous_target) &&
        (dac->fsm.current_id == DAC_FSM_STATE_RUNNING)) {
        FSM_Transition(&dac->fsm, DAC_FSM_STATE_VOLUME);
    }
}

uint8_t DAC_GetVolume(void) {
    return dac->volume;
}

void DAC_Mute(const bool muted) {
    if (dac->mute_override == muted) {
        return;
    }

    const uint16_t previous_target = dac_volume_target();
    dac->mute_override = muted;

    if ((dac_volume_target() != previous_target) &&
        (dac->fsm.current_id == DAC_FSM_STATE_RUNNING)) {
        FSM_Transition(&dac->fsm, DAC_FSM_STATE_VOLUME);
    }
}

bool DAC_IsMuted(void) {
    return dac->mute_override;
}
