#ifndef EDGEWORK_MODE_TWOFA_H
#define EDGEWORK_MODE_TWOFA_H

#include <stdbool.h>
#include <stdint.h>

#include "edgework/data.h"
#include "i2c/i2c.h"
#include "input_manager/input_manager.h"

typedef struct Mode_Definition Mode_Definition;

typedef struct {
    IM_EventQueue input_queue;
    IM_DigitalChannelState button_channel_state[1];
    IM_DigitalInputState button_input_state;
    IM_DigitalInputConfig button_input_config;
    IM_Handle button_handle;

    I2C_Transaction lcd_transaction;
    uint8_t lcd_command_data[2];
    uint8_t lcd_transaction_data[12];
    uint8_t lcd_initialization_step;
    uint32_t lcd_initialization_due_ms;
    bool lcd_initialized;
    bool lcd_initialization_queued;
    bool display_written;
    bool has_display_state;
    edgework_state_t displayed_state;
} TwoFA_Data;

extern Mode_Definition twofa_mode;

#endif // EDGEWORK_MODE_TWOFA_H
