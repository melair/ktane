#include "argb.h"

#include <stddef.h>

#include "argb_platform.h"

typedef enum {
    ARGB_STATE_IDLE = 0,
    ARGB_STATE_UPDATING,
} ARGB_STATE;

typedef enum {
    DMA_HALF_COMPLETE,
    DMA_TRANSFER_COMPLETE,
} DMA_EVENT;

typedef struct {
    ARGB_Strip *strip_list;
    uint8_t max_brightness;
    ARGB_STATE current_state;
    ARGB_Strip *current_strip;
    uint8_t current_led_idx;
    uint8_t queued_led_count;
    uint8_t pulse_high;
    uint8_t pulse_low;
    uint8_t pwm_buffer[BITS_PER_LED * LEDS_IN_TRANSPORT_BUFFER];
} argb_t;

static argb_t argb = {
    .max_brightness = ARGB_BRIGHTNESS_MAX,
    .current_state = ARGB_STATE_IDLE,
};

#define ARGB_FREQUENCY_HZ 800000U

bool ARGB_Init(void) {
    if (ARGB_HARDWARE.timer_clock_hz < ARGB_FREQUENCY_HZ) {
        return false;
    }

    const uint32_t period_ticks = ARGB_HARDWARE.timer_clock_hz / ARGB_FREQUENCY_HZ;
    const uint32_t pulse_low = (period_ticks + 1U) / 3U;
    const uint32_t pulse_high = ((2U * period_ticks) + 1U) / 3U;
    if ((period_ticks > UINT16_MAX) || (pulse_high > UINT8_MAX)) {
        return false;
    }

    argb.pulse_low = (uint8_t) pulse_low;
    argb.pulse_high = (uint8_t) pulse_high;

    return ARGB_PLATFORM.init(&ARGB_HARDWARE, (uint16_t) period_ticks);
}

void ARGB_Set(ARGB_Strip *strip, const uint8_t idx, const uint8_t r, const uint8_t g, const uint8_t b) {
    if ((strip == NULL) || (idx >= strip->count)) {
        return;
    }

    if ((strip->leds[idx].r == r) &&
        (strip->leds[idx].g == g) &&
        (strip->leds[idx].b == b)) {
        return;
    }

    strip->leds[idx].r = r;
    strip->leds[idx].g = g;
    strip->leds[idx].b = b;
    strip->dirty = true;
}

void ARGB_Fill(ARGB_Strip *strip, const uint8_t r, const uint8_t g, const uint8_t b) {
    if ((strip == NULL) || (strip->leds == NULL)) {
        return;
    }

    for (uint8_t idx = 0; idx < strip->count; idx++) {
        if ((strip->leds[idx].r != r) ||
            (strip->leds[idx].g != g) ||
            (strip->leds[idx].b != b)) {
            strip->dirty = true;
        }

        strip->leds[idx].r = r;
        strip->leds[idx].g = g;
        strip->leds[idx].b = b;
    }
}

void ARGB_Set_Brightness(const uint8_t max_brightness) {
    if (argb.max_brightness == max_brightness) {
        return;
    }

    argb.max_brightness = max_brightness;

    for (ARGB_Strip *strip = argb.strip_list; strip != NULL; strip = strip->next) {
        strip->dirty = true;
    }
}

static void populate_pwm_data(const ARGB_Strip *strip, const uint8_t led_idx, const uint8_t pwm_offset) {
    uint8_t colour_order = strip->colour_order;
    uint8_t buffer_idx = pwm_offset;

    for (uint8_t c = 0; c < COLOURS_PER_LED; c++) {
        const uint8_t colour = colour_order & 0x03U;
        colour_order >>= 2U;

        uint8_t colour_val;
        switch (colour) {
            default:
            case COLOUR_RED:
                colour_val = strip->leds[led_idx].r;
                break;
            case COLOUR_GREEN:
                colour_val = strip->leds[led_idx].g;
                break;
            case COLOUR_BLUE:
                colour_val = strip->leds[led_idx].b;
                break;
        }

        if ((strip->max_brightness != ARGB_BRIGHTNESS_MAX) ||
            (argb.max_brightness != ARGB_BRIGHTNESS_MAX)) {
            colour_val = (uint8_t) (((uint32_t) colour_val * strip->max_brightness * argb.max_brightness) /
                                    (ARGB_BRIGHTNESS_MAX * ARGB_BRIGHTNESS_MAX));
        }

        for (int8_t bit = 7; bit >= 0; bit--) {
            argb.pwm_buffer[buffer_idx++] = (colour_val & (1U << bit)) != 0U
                                                ? argb.pulse_high
                                                : argb.pulse_low;
        }
    }
}

static void clear_pwm_data(const uint8_t pwm_offset) {
    for (uint8_t idx = 0; idx < BITS_PER_LED; idx++) {
        argb.pwm_buffer[pwm_offset + idx] = 0U;
    }
}

static bool load_next_led(const uint8_t pwm_offset) {
    while (argb.current_strip != NULL) {
        if (argb.current_led_idx < argb.current_strip->count) {
            populate_pwm_data(argb.current_strip, argb.current_led_idx++, pwm_offset);
            argb.queued_led_count++;
            return true;
        }

        argb.current_strip = argb.current_strip->next;
        argb.current_led_idx = 0U;
    }

    clear_pwm_data(pwm_offset);
    return false;
}

static void handle_transfer_event(const DMA_EVENT event) {
    if (argb.current_state != ARGB_STATE_UPDATING) {
        return;
    }

    const uint8_t offset = event == DMA_HALF_COMPLETE ? 0U : BITS_PER_LED;

    if (argb.queued_led_count > 0U) {
        argb.queued_led_count--;
    }

    load_next_led(offset);

    if (argb.queued_led_count == 0U) {
        ARGB_PLATFORM.stop();
        argb.current_state = ARGB_STATE_IDLE;
    }
}

void ARGB_Platform_NotifyHalfComplete(void) {
    handle_transfer_event(DMA_HALF_COMPLETE);
}

void ARGB_Platform_NotifyTransferComplete(void) {
    handle_transfer_event(DMA_TRANSFER_COMPLETE);
}

void ARGB_Add_Strip(ARGB_Strip *strip) {
    if ((strip == NULL) || (strip->leds == NULL) || (strip->count == 0U)) {
        return;
    }

    strip->next = NULL;
    strip->dirty = true;

    if (argb.strip_list == NULL) {
        argb.strip_list = strip;
    } else {
        ARGB_Strip *tail = argb.strip_list;
        while (tail->next != NULL) {
            tail = tail->next;
        }

        tail->next = strip;
    }
}

static bool strips_are_dirty(void) {
    for (const ARGB_Strip *strip = argb.strip_list; strip != NULL; strip = strip->next) {
        if (strip->dirty) {
            return true;
        }
    }

    return false;
}

static void clear_strip_dirty_flags(void) {
    for (ARGB_Strip *strip = argb.strip_list; strip != NULL; strip = strip->next) {
        strip->dirty = false;
    }
}

static void set_strip_dirty_flags(void) {
    for (ARGB_Strip *strip = argb.strip_list; strip != NULL; strip = strip->next) {
        strip->dirty = true;
    }
}

void ARGB_Service(void) {
    if ((argb.current_state != ARGB_STATE_IDLE) || (argb.strip_list == NULL) || !strips_are_dirty()) {
        return;
    }

    clear_strip_dirty_flags();

    argb.current_strip = argb.strip_list;
    argb.current_led_idx = 0U;
    argb.queued_led_count = 0U;
    argb.current_state = ARGB_STATE_UPDATING;

    load_next_led(0U);
    load_next_led(BITS_PER_LED);

    if (argb.queued_led_count == 0U) {
        argb.current_state = ARGB_STATE_IDLE;
        return;
    }

    if (!ARGB_PLATFORM.start(argb.pwm_buffer, BITS_PER_LED * LEDS_IN_TRANSPORT_BUFFER)) {
        set_strip_dirty_flags();
        argb.current_state = ARGB_STATE_IDLE;
    }
}

void ARGB_Platform_DMA_IRQHandler(void) {
    ARGB_PLATFORM.dma_irq();
}
