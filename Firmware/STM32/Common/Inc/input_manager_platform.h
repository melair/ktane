#ifndef INPUT_MANAGER_PLATFORM_H
#define INPUT_MANAGER_PLATFORM_H

#include <stdbool.h>
#include <stdint.h>

#include "input_manager.h"

struct IM_RotaryHardware {
    void *timer_instance;
    void *timer_handle;

    GPIO_PinDef channel_a;
    uint32_t channel_a_alternate;

    GPIO_PinDef channel_b;
    uint32_t channel_b_alternate;

    void (*enable_timer_clock)(void);
};

typedef struct {
    bool (*analogue_init)(void);
    bool (*analogue_start)(uint32_t adc_channel);
    bool (*analogue_poll)(uint16_t *value);

    bool (*rotary_init)(const IM_RotaryHardware *hardware, bool enable_internal_pullups);
    uint16_t (*rotary_count)(const IM_RotaryHardware *hardware);
} IM_Platform;

// Exactly one MCU-family platform implementation must provide this instance.
extern const IM_Platform IM_PLATFORM;

#endif //INPUT_MANAGER_PLATFORM_H
