#include "indicator.h"
#include "argb.h"
#include "stm32h5xx_hal.h"

typedef struct {
    ARGB_LED colour;
    uint32_t duration_ms;
} IndicatorFrame;

typedef struct {
    const IndicatorFrame *frames;
    uint8_t frame_count;
    IndicatorMode next_mode;
} IndicatorAnimation;

typedef struct {
    const IndicatorAnimation *animation;
    uint8_t frame_index;
    uint32_t next_frame_at;
} indicator_t;

#define FRAME_COUNT(frames) ((uint8_t) (sizeof(frames) / sizeof((frames)[0])))

static const IndicatorFrame indicator_off_frames[] = {
    {.colour = {0, 0, 0}, .duration_ms = 1000},
};

static const IndicatorFrame indicator_id_frames[] = {
    {.colour = {0, 0, 0xff}, .duration_ms = 500},
    {.colour = {0, 0, 0x00}, .duration_ms = 500},
};

static const IndicatorFrame indicator_error_frames[] = {
    {.colour = {0xff, 0, 0}, .duration_ms = 250},
    {.colour = {0, 0, 0x00}, .duration_ms = 250},
};

static const IndicatorFrame indicator_prepare_frames[] = {
    {.colour = {0, 0xff, 0xff}, .duration_ms = 500},
    {.colour = {0, 0, 0}, .duration_ms = 500},
};

static const IndicatorFrame indicator_strike_frames[] = {
    {.colour = {0xff, 0, 0}, .duration_ms = 1000},
};

static const IndicatorFrame indicator_solved_frames[] = {
    {.colour = {0, 0xff, 0}, .duration_ms = 1000},
};

static const IndicatorAnimation indicator_animations[INDICATOR_MODE_COUNT] = {
    [INDICATOR_OFF] = {
        .frames = indicator_off_frames,
        .frame_count = FRAME_COUNT(indicator_off_frames),
        .next_mode = INDICATOR_OFF,
    },
    [INDICATOR_ID] = {
        .frames = indicator_id_frames,
        .frame_count = FRAME_COUNT(indicator_id_frames),
        .next_mode = INDICATOR_ID,
    },
    [INDICATOR_ERROR] = {
        .frames = indicator_error_frames,
        .frame_count = FRAME_COUNT(indicator_error_frames),
        .next_mode = INDICATOR_ERROR,
    },
    [INDICATOR_PREPARE] = {
        .frames = indicator_prepare_frames,
        .frame_count = FRAME_COUNT(indicator_prepare_frames),
        .next_mode = INDICATOR_PREPARE,
    },
    [INDICATOR_STRIKE] = {
        .frames = indicator_strike_frames,
        .frame_count = FRAME_COUNT(indicator_strike_frames),
        .next_mode = INDICATOR_OFF,
    },
    [INDICATOR_SOLVED] = {
        .frames = indicator_solved_frames,
        .frame_count = FRAME_COUNT(indicator_solved_frames),
        .next_mode = INDICATOR_SOLVED,
    },
};

static ARGB_LED indicator_led = {0};

static ARGB_Strip indicator_strip = {
    .count = 1,
    .colour_order = COLOUR_ORDER_GRB,
    .max_brightness = ARGB_BRIGHTNESS_MAX,
    .leds = &indicator_led,
};

static indicator_t indicator = {0};

static void render_current_frame(void) {
    const IndicatorFrame *frame = &indicator.animation->frames[indicator.frame_index];

    ARGB_Set(&indicator_strip, 0, frame->colour.r, frame->colour.g, frame->colour.b);
    indicator.next_frame_at = HAL_GetTick() + frame->duration_ms;
}

void Indicator_Init(void) {
    ARGB_Add_Strip(&indicator_strip);

    Indicator_Set(INDICATOR_OFF);
}

void Indicator_Service(void) {
    if ((indicator.animation != NULL) &&
        ((int32_t) (HAL_GetTick() - indicator.next_frame_at) >= 0)) {
        indicator.frame_index++;
        if (indicator.frame_index >= indicator.animation->frame_count) {
            Indicator_Set(indicator.animation->next_mode);
            return;
        }

        render_current_frame();
    }
}

void Indicator_Set(const IndicatorMode mode) {
    if (mode >= INDICATOR_MODE_COUNT) {
        return;
    }

    indicator.animation = &indicator_animations[mode];
    indicator.frame_index = 0;
    render_current_frame();
}
