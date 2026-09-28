#ifndef SIMON_H
#define SIMON_H

#include "input_manager/input_manager.h"
#include "mode_fsm.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t startup_lamp_index;
    uint8_t startup_loop_count;
    bool startup_lamp_on;
    uint32_t startup_next_change_ms;
    bool attract_lamp_on[4];
    uint32_t attract_lamp_until_ms[4];
    IM_EventQueue button_queue;
    IM_DigitalChannelState button_channel_state[4];
    IM_DigitalInputState button_input_state;
    IM_DigitalInputConfig button_input_config;
    IM_Handle button_handle;
} Simon_Data;

extern Mode_Definition simon_mode;

#ifdef __cplusplus
}
#endif

#endif //SIMON_H
