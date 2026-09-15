#include "mode/twofa/twofa.h"

#include "i2c/i2c.h"
#include "main.h"
#include "mode.h"
#include "slot.h"
#include "sys/gpio.h"

#include <string.h>

#define TWOFA_LCD_I2C_ADDRESS             0x38U
#define TWOFA_LCD_COMMAND_DATA             0x80U
#define TWOFA_LCD_COMMAND_DRIVE_MODE       0x82U
#define TWOFA_LCD_COMMAND_SYSTEM_MODE      0x84U
#define TWOFA_LCD_COMMAND_FRAME_FREQUENCY  0x86U
#define TWOFA_LCD_COMMAND_BLINK_FREQUENCY  0x88U
#define TWOFA_LCD_COMMAND_INTERNAL_VLCD    0x8AU
#define TWOFA_LCD_INITIALIZATION_STEPS     6U
#define TWOFA_LCD_POWER_ON_DELAY_MS         1U

#define TWOFA_LCD_DRIVE_MODE_1_4_DUTY_1_3_BIAS 0x00U
#define TWOFA_LCD_FRAME_FREQUENCY_80HZ          0x00U
/* VE=1, DA=1: approximately 0.94 x VDD at 1/3 bias. */
#define TWOFA_LCD_INTERNAL_VLCD_CONTRAST         0x11U
#define TWOFA_LCD_SYSTEM_OSCILLATOR_DISPLAY_ON  0x03U
#define TWOFA_LCD_BLINK_OFF                      0x00U
#define TWOFA_LCD_BLINK_2HZ                      0x01U

#define TWOFA_LCD_RAM_SIZE             10U
#define TWOFA_LCD_RAM_DATA_OFFSET      2U

typedef enum {
    TWOFA_SEGMENT_A = 1U << 0,
    TWOFA_SEGMENT_B = 1U << 1,
    TWOFA_SEGMENT_C = 1U << 2,
    TWOFA_SEGMENT_D = 1U << 3,
    TWOFA_SEGMENT_E = 1U << 4,
    TWOFA_SEGMENT_F = 1U << 5,
    TWOFA_SEGMENT_G = 1U << 6,
} TwoFA_SevenSegment;

typedef struct {
    uint8_t character;
    uint8_t segments;
} TwoFA_CharacterSegments;

typedef struct {
    uint8_t segment;
    uint8_t pin_offset;
    uint8_t common;
} TwoFA_SegmentLocation;

static TwoFA_Data *const twofa = &mode_data.mode.twofa;
static const GPIO_PinDef twofa_button_pin = {GPIO4_Port, GPIO4_Pin};

/* The 2FA LCD PCB reverses the glass segment and common buses. */
static const uint8_t twofa_glass_segment_map[] = {
    [0] = 18U, [1] = 17U, [2] = 16U, [3] = 15U, [4] = 14U, [5] = 13U,
    [6] = 12U, [7] = 11U, [8] = 10U, [9] = 9U, [10] = 8U, [11] = 7U,
    [12] = 6U,
};

static const uint8_t twofa_glass_common_map[] = {
    [0] = 3U, [1] = 2U, [2] = 1U, [3] = 0U,
};

static const uint8_t twofa_digit_segments[10] = {
    [0] = TWOFA_SEGMENT_A | TWOFA_SEGMENT_B | TWOFA_SEGMENT_C |
          TWOFA_SEGMENT_D | TWOFA_SEGMENT_E | TWOFA_SEGMENT_F,
    [1] = TWOFA_SEGMENT_B | TWOFA_SEGMENT_C,
    [2] = TWOFA_SEGMENT_A | TWOFA_SEGMENT_B | TWOFA_SEGMENT_D |
          TWOFA_SEGMENT_E | TWOFA_SEGMENT_G,
    [3] = TWOFA_SEGMENT_A | TWOFA_SEGMENT_B | TWOFA_SEGMENT_C |
          TWOFA_SEGMENT_D | TWOFA_SEGMENT_G,
    [4] = TWOFA_SEGMENT_B | TWOFA_SEGMENT_C | TWOFA_SEGMENT_F |
          TWOFA_SEGMENT_G,
    [5] = TWOFA_SEGMENT_A | TWOFA_SEGMENT_C | TWOFA_SEGMENT_D |
          TWOFA_SEGMENT_F | TWOFA_SEGMENT_G,
    [6] = TWOFA_SEGMENT_A | TWOFA_SEGMENT_C | TWOFA_SEGMENT_D |
          TWOFA_SEGMENT_E | TWOFA_SEGMENT_F | TWOFA_SEGMENT_G,
    [7] = TWOFA_SEGMENT_A | TWOFA_SEGMENT_B | TWOFA_SEGMENT_C,
    [8] = TWOFA_SEGMENT_A | TWOFA_SEGMENT_B | TWOFA_SEGMENT_C |
          TWOFA_SEGMENT_D | TWOFA_SEGMENT_E | TWOFA_SEGMENT_F |
          TWOFA_SEGMENT_G,
    [9] = TWOFA_SEGMENT_A | TWOFA_SEGMENT_B | TWOFA_SEGMENT_C |
          TWOFA_SEGMENT_D | TWOFA_SEGMENT_F | TWOFA_SEGMENT_G,
};

static const TwoFA_CharacterSegments twofa_glyph_segments[] = {
    {'A', TWOFA_SEGMENT_A | TWOFA_SEGMENT_B | TWOFA_SEGMENT_C |
          TWOFA_SEGMENT_E | TWOFA_SEGMENT_F | TWOFA_SEGMENT_G},
    {'B', TWOFA_SEGMENT_C | TWOFA_SEGMENT_D | TWOFA_SEGMENT_E |
          TWOFA_SEGMENT_F | TWOFA_SEGMENT_G},
    {'C', TWOFA_SEGMENT_A | TWOFA_SEGMENT_D | TWOFA_SEGMENT_E |
          TWOFA_SEGMENT_F},
    {'D', TWOFA_SEGMENT_B | TWOFA_SEGMENT_C | TWOFA_SEGMENT_D |
          TWOFA_SEGMENT_E | TWOFA_SEGMENT_G},
    {'E', TWOFA_SEGMENT_A | TWOFA_SEGMENT_D | TWOFA_SEGMENT_E |
          TWOFA_SEGMENT_F | TWOFA_SEGMENT_G},
    {'F', TWOFA_SEGMENT_A | TWOFA_SEGMENT_E | TWOFA_SEGMENT_F |
          TWOFA_SEGMENT_G},
    {'S', TWOFA_SEGMENT_A | TWOFA_SEGMENT_C | TWOFA_SEGMENT_D |
          TWOFA_SEGMENT_F | TWOFA_SEGMENT_G},
    {'L', TWOFA_SEGMENT_D | TWOFA_SEGMENT_E | TWOFA_SEGMENT_F},
    {'O', TWOFA_SEGMENT_C | TWOFA_SEGMENT_D | TWOFA_SEGMENT_E |
          TWOFA_SEGMENT_G},
    {'T', TWOFA_SEGMENT_D | TWOFA_SEGMENT_E | TWOFA_SEGMENT_F |
          TWOFA_SEGMENT_G},
};

/* Segment positions relative to the first glass pin for a digit. */
static const TwoFA_SegmentLocation twofa_segment_locations[] = {
    {TWOFA_SEGMENT_A, 1U, 0U},
    {TWOFA_SEGMENT_B, 1U, 1U},
    {TWOFA_SEGMENT_C, 1U, 2U},
    {TWOFA_SEGMENT_D, 0U, 3U},
    {TWOFA_SEGMENT_E, 0U, 2U},
    {TWOFA_SEGMENT_F, 0U, 0U},
    {TWOFA_SEGMENT_G, 0U, 1U},
};

static I2C_Transaction *twofa_lcd_i2c_complete(I2C_Transaction *transaction);
static I2C_Transaction *twofa_lcd_blink_complete(I2C_Transaction *transaction);
static I2C_Transaction *twofa_lcd_display_complete(I2C_Transaction *transaction);

static uint8_t twofa_hex_digit(uint8_t value) {
    value &= 0x0FU;
    return value < 10U ? (uint8_t) ('0' + value) : (uint8_t) ('A' + value - 10U);
}

static void twofa_lcd_ram_clear(void) {
    twofa->lcd_transaction_data[0] = TWOFA_LCD_COMMAND_DATA;
    twofa->lcd_transaction_data[1] = 0U;
    memset(&twofa->lcd_transaction_data[TWOFA_LCD_RAM_DATA_OFFSET], 0,
           TWOFA_LCD_RAM_SIZE);
}

/* glass_pin and common use the numbering from the LCD glass table. */
static void twofa_lcd_segment_set(uint8_t glass_pin, uint8_t common) {
    const uint8_t segment = twofa_glass_segment_map[glass_pin - 1U];
    const uint8_t lcd_common = twofa_glass_common_map[common];
    const uint8_t address = segment / 2U;
    const uint8_t bit = ((segment & 1U) == 0U ? 0U : 4U) + lcd_common;

    twofa->lcd_transaction_data[TWOFA_LCD_RAM_DATA_OFFSET + address] |= 1U << bit;
}

static uint8_t twofa_character_segments(uint8_t value) {
    if (value < 10U) {
        return twofa_digit_segments[value];
    }

    if ((value >= '0') && (value <= '9')) {
        return twofa_digit_segments[value - '0'];
    }

    for (uint8_t index = 0U; index < sizeof(twofa_glyph_segments) /
                                             sizeof(twofa_glyph_segments[0]);
         index++) {
        if (twofa_glyph_segments[index].character == value) {
            return twofa_glyph_segments[index].segments;
        }
    }

    return 0U;
}

static void twofa_lcd_digit_set(uint8_t digit, uint8_t value) {
    const uint8_t segments = twofa_character_segments(value);
    const uint8_t first_glass_pin = 2U + (digit * 2U);

    for (uint8_t index = 0U; index < sizeof(twofa_segment_locations) /
                                         sizeof(twofa_segment_locations[0]);
         index++) {
        const TwoFA_SegmentLocation *location = &twofa_segment_locations[index];

        if ((segments & location->segment) != 0U) {
            twofa_lcd_segment_set(first_glass_pin + location->pin_offset,
                                  location->common);
        }
    }
}

static void twofa_lcd_icons_set(const edgework_state_t *state) {
    if (state->twofa.icons.t1 != 0U) {
        twofa_lcd_segment_set(1U, 0U);
    }
    if (state->twofa.icons.t2 != 0U) {
        twofa_lcd_segment_set(1U, 1U);
    }
    if (state->twofa.icons.t3 != 0U) {
        twofa_lcd_segment_set(1U, 2U);
    }
    if (state->twofa.icons.t4 != 0U) {
        twofa_lcd_segment_set(1U, 3U);
    }
    if (state->twofa.icons.col1 != 0U) {
        twofa_lcd_segment_set(6U, 3U);
    }
    if (state->twofa.icons.dot != 0U) {
        twofa_lcd_segment_set(9U, 3U);
    }
}

static void twofa_lcd_display_prepare(const edgework_state_t *state) {
    const bool blink = (state->identify != 0U) || (state->twofa.flags.flashing != 0U);

    twofa_lcd_ram_clear();

    if (state->identify != 0U) {
        const uint8_t slot = Slot_Get();
        const uint8_t identify_text[] = {
            'S', 'L', 'O', 'T', twofa_hex_digit(slot >> 4U), twofa_hex_digit(slot),
        };

        for (uint8_t digit = 0U; digit < sizeof(identify_text); digit++) {
            twofa_lcd_digit_set(digit, identify_text[digit]);
        }
    } else {
        for (uint8_t digit = 0U; digit < sizeof(state->twofa.value); digit++) {
            twofa_lcd_digit_set(digit, state->twofa.value[digit]);
        }
        twofa_lcd_icons_set(state);
    }

    twofa->lcd_transaction = (I2C_Transaction) {
        .operation = I2C_OPERATION_WRITE,
        .address = TWOFA_LCD_I2C_ADDRESS,
        .tx_data = twofa->lcd_command_data,
        .tx_size = sizeof(twofa->lcd_command_data),
        .callback = twofa_lcd_blink_complete,
    };
    twofa->lcd_command_data[0] = TWOFA_LCD_COMMAND_BLINK_FREQUENCY;
    twofa->lcd_command_data[1] = blink ? TWOFA_LCD_BLINK_2HZ : TWOFA_LCD_BLINK_OFF;
}

static void twofa_lcd_clear_prepare(void) {
    twofa_lcd_ram_clear();
    twofa->lcd_transaction = (I2C_Transaction) {
        .operation = I2C_OPERATION_WRITE,
        .address = TWOFA_LCD_I2C_ADDRESS,
        .tx_data = twofa->lcd_command_data,
        .tx_size = sizeof(twofa->lcd_command_data),
        .callback = twofa_lcd_blink_complete,
    };
    twofa->lcd_command_data[0] = TWOFA_LCD_COMMAND_BLINK_FREQUENCY;
    twofa->lcd_command_data[1] = TWOFA_LCD_BLINK_OFF;
}

static void twofa_lcd_initialization_prepare(void) {
    uint16_t transaction_size = 2U;

    switch (twofa->lcd_initialization_step) {
        case 0U:
            twofa->lcd_transaction_data[0] = TWOFA_LCD_COMMAND_DRIVE_MODE;
            twofa->lcd_transaction_data[1] = TWOFA_LCD_DRIVE_MODE_1_4_DUTY_1_3_BIAS;
            break;
        case 1U:
            twofa->lcd_transaction_data[0] = TWOFA_LCD_COMMAND_FRAME_FREQUENCY;
            twofa->lcd_transaction_data[1] = TWOFA_LCD_FRAME_FREQUENCY_80HZ;
            break;
        case 2U:
            twofa->lcd_transaction_data[0] = TWOFA_LCD_COMMAND_INTERNAL_VLCD;
            twofa->lcd_transaction_data[1] = TWOFA_LCD_INTERNAL_VLCD_CONTRAST;
            break;
        case 3U:
            twofa_lcd_ram_clear();
            transaction_size = sizeof(twofa->lcd_transaction_data);
            break;
        case 4U:
            twofa->lcd_transaction_data[0] = TWOFA_LCD_COMMAND_SYSTEM_MODE;
            twofa->lcd_transaction_data[1] = TWOFA_LCD_SYSTEM_OSCILLATOR_DISPLAY_ON;
            break;
        case 5U:
            /* Reassert IVA after display enable for the externally observable VLCD pin. */
            twofa->lcd_transaction_data[0] = TWOFA_LCD_COMMAND_INTERNAL_VLCD;
            twofa->lcd_transaction_data[1] = TWOFA_LCD_INTERNAL_VLCD_CONTRAST;
            break;
        default:
            Error_Handler();
            return;
    }

    twofa->lcd_transaction = (I2C_Transaction) {
        .operation = I2C_OPERATION_WRITE,
        .address = TWOFA_LCD_I2C_ADDRESS,
        .tx_data = twofa->lcd_transaction_data,
        .tx_size = transaction_size,
        .callback = twofa_lcd_i2c_complete,
    };
}

static I2C_Transaction *twofa_lcd_i2c_complete(I2C_Transaction *transaction) {
    if (transaction->status != I2C_STATUS_SUCCESS) {
        Error_Handler();
        return NULL;
    }

    twofa->lcd_initialization_step++;
    if (twofa->lcd_initialization_step >= TWOFA_LCD_INITIALIZATION_STEPS) {
        twofa->lcd_initialized = true;
        return NULL;
    }

    twofa_lcd_initialization_prepare();
    return &twofa->lcd_transaction;
}

static void twofa_init_enter(FSM *fsm) {
    twofa->button_input_state = (IM_DigitalInputState) {
        .channels = twofa->button_channel_state,
    };
    twofa->button_input_config = (IM_DigitalInputConfig) {
        .rows = &twofa_button_pin,
        .row_count = 1U,
        .queue = &twofa->input_queue,
        .event_mask = IM_EVENT_DOWN,
        .state = &twofa->button_input_state,
        .scan_period_ms = 10U,
        .debounce_ms = 30U,
        .pull = IM_DIGITAL_INPUT_PULL_UP,
        .active_high = false,
    };

    twofa->button_handle = IM_RegisterDigital(&twofa->button_input_config);
    if (twofa->button_handle == IM_INVALID_HANDLE) {
        Error_Handler();
        return;
    }

    twofa->lcd_initialized = false;
    twofa->lcd_initialization_queued = false;
    twofa->lcd_initialization_due_ms = HAL_GetTick() + TWOFA_LCD_POWER_ON_DELAY_MS;

    (void) FSM_Transition(fsm, EDGEWORK_MODE_STATE_STARTUP);
}

static void twofa_startup_service(FSM *fsm) {
    if (!twofa->lcd_initialization_queued) {
        if ((int32_t) (HAL_GetTick() - twofa->lcd_initialization_due_ms) < 0) {
            return;
        }

        twofa->lcd_initialization_step = 0U;
        twofa_lcd_initialization_prepare();
        I2C_Queue(&twofa->lcd_transaction);
        twofa->lcd_initialization_queued = true;
        return;
    }

    if (twofa->lcd_initialized) {
        (void) FSM_Transition(fsm, EDGEWORK_MODE_STATE_IDLE);
    }
}

static I2C_Transaction *twofa_lcd_blink_complete(I2C_Transaction *transaction) {
    if (transaction->status != I2C_STATUS_SUCCESS) {
        Error_Handler();
        return NULL;
    }

    twofa->lcd_transaction.tx_data = twofa->lcd_transaction_data;
    twofa->lcd_transaction.tx_size = TWOFA_LCD_RAM_DATA_OFFSET + TWOFA_LCD_RAM_SIZE;
    twofa->lcd_transaction.callback = twofa_lcd_display_complete;
    return &twofa->lcd_transaction;
}

static I2C_Transaction *twofa_lcd_display_complete(I2C_Transaction *transaction) {
    if (transaction->status != I2C_STATUS_SUCCESS) {
        Error_Handler();
        return NULL;
    }

    twofa->display_written = true;
    return NULL;
}

static void twofa_display_enter(FSM *fsm) {
    twofa->displayed_state = mode_data.desired_state;
    twofa->has_display_state = true;
    if (twofa->displayed_state.twofa.flags.active_display == 0U) {
        mode_data.current_state = twofa->displayed_state;
        (void) FSM_Transition(fsm, EDGEWORK_MODE_STATE_IDLE);
        return;
    }

    twofa->display_written = false;
    twofa_lcd_display_prepare(&twofa->displayed_state);
    I2C_Queue(&twofa->lcd_transaction);
}

static void twofa_display_service(FSM *fsm) {
    if (!twofa->display_written) {
        return;
    }

    mode_data.current_state = twofa->displayed_state;
    (void) FSM_Transition(fsm, EDGEWORK_MODE_STATE_IDLE);
}

static void twofa_clear_enter(FSM *fsm) {
    (void) fsm;

    twofa->has_display_state = false;
    twofa->display_written = false;
    twofa_lcd_clear_prepare();
    I2C_Queue(&twofa->lcd_transaction);
}

static void twofa_clear_service(FSM *fsm) {
    if (twofa->display_written) {
        (void) FSM_Transition(fsm, EDGEWORK_MODE_STATE_IDLE);
    }
}

static void twofa_idle_service(FSM *fsm) {
    IM_Event event;
    while (IM_EventQueue_Read(&twofa->input_queue, &event)) {
        if (twofa->has_display_state && (event.event == IM_EVENT_DOWN) &&
            (mode_data.desired_state.twofa.flags.active_display == 0U)) {
            mode_data.desired_state.twofa.flags.active_display = 1U;
            (void) FSM_Transition(fsm, EDGEWORK_MODE_STATE_DISPLAY);
            return;
        }
    }
}

static Mode_Callbacks twofa_state_callbacks[EDGEWORK_MODE_STATE_COUNT] = {
    [EDGEWORK_MODE_STATE_INIT] = {
        .enter = twofa_init_enter,
    },
    [EDGEWORK_MODE_STATE_STARTUP] = {
        .service = twofa_startup_service,
    },
    [EDGEWORK_MODE_STATE_CLEAR] = {
        .enter = twofa_clear_enter,
        .service = twofa_clear_service,
    },
    [EDGEWORK_MODE_STATE_IDLE] = {
        .service = twofa_idle_service,
    },
    [EDGEWORK_MODE_STATE_DISPLAY] = {
        .enter = twofa_display_enter,
        .service = twofa_display_service,
    },
};

Mode_Definition twofa_mode = {
    .state_callbacks = twofa_state_callbacks,
    .always_service = NULL,
    .display_acknowledges_state = true,
};
