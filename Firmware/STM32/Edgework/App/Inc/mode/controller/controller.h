#ifndef EDGEWORK_MODE_CONTROLLER_H
#define EDGEWORK_MODE_CONTROLLER_H

#include <stdint.h>

#include "input_manager/input_manager.h"

typedef struct Mode_Definition Mode_Definition;

typedef struct {
    IM_DigitalChannelState power_channel_state[1];
    IM_DigitalInputState power_input_state;
    IM_DigitalInputConfig power_input_config;
    IM_Handle power_input_handle;
} Controller_Data;

extern Mode_Definition controller_mode;

#endif // EDGEWORK_MODE_CONTROLLER_H
