#include "mode/support/timer/timer.h"
#include "mode.h"
#include "mode_fsm.h"
#include "sys/gpio.h"
#include "sys/rng.h"
#include "sys/spi.h"

#define TIMER_ROTARY_COUNTS_PER_DETENT 4
#define TIMER_LEDS_PER_DIGIT 11
#define TIMER_STARTUP_DELAY_MS 1000u
#define TIMER_STARTUP_SEGMENT_MS 250u
#define TIMER_STARTUP_SEGMENT_REPEATS 2u
#define TIMER_ATTRACT_TICK_MS 10u
#define TIMER_ATTRACT_MIN_SECONDS 90u
#define TIMER_ATTRACT_MAX_SECONDS 180u
#define TIMER_ATTRACT_STRIKES_MIN_DELAY_MS 5000u
#define TIMER_ATTRACT_STRIKES_MAX_DELAY_MS 10000u
#define TIMER_ATTRACT_STRIKES_FLASH_HALF_PERIOD_MS 167u

#define TIMER_DIGIT_1_LED_OFFSET 0
#define TIMER_DIGIT_2_LED_OFFSET (TIMER_DIGIT_1_LED_OFFSET + TIMER_LEDS_PER_DIGIT)
#define TIMER_DECIMAL_POINT_LED_OFFSET (TIMER_DIGIT_2_LED_OFFSET + TIMER_LEDS_PER_DIGIT)
#define TIMER_COLON_LOWER_LED_OFFSET (TIMER_DECIMAL_POINT_LED_OFFSET + 1)
#define TIMER_COLON_UPPER_LED_OFFSET (TIMER_COLON_LOWER_LED_OFFSET + 1)
#define TIMER_DIGIT_3_LED_OFFSET (TIMER_COLON_UPPER_LED_OFFSET + 1)
#define TIMER_DIGIT_4_LED_OFFSET (TIMER_DIGIT_3_LED_OFFSET + TIMER_LEDS_PER_DIGIT)

#define ARRAY_COUNT(array) (sizeof(array) / sizeof((array)[0]))

static Timer_Data *const timer = &mode_data.mode.timer;

enum {
    TIMER_ATTRACT_STRIKES_WAIT_FIRST,
    TIMER_ATTRACT_STRIKES_WAIT_SECOND,
    TIMER_ATTRACT_STRIKES_FLASHING,
};

static const Timer_Glyph timer_digit_led_segments[TIMER_LEDS_PER_DIGIT] = {
    TIMER_SEGMENT_F,
    TIMER_SEGMENT_F,
    TIMER_SEGMENT_A,
    TIMER_SEGMENT_B,
    TIMER_SEGMENT_B,
    TIMER_SEGMENT_G,
    TIMER_SEGMENT_E,
    TIMER_SEGMENT_E,
    TIMER_SEGMENT_D,
    TIMER_SEGMENT_C,
    TIMER_SEGMENT_C,
};

static const Timer_Glyph timer_digit_glyphs[10] = {
    TIMER_DIGIT_0,
    TIMER_DIGIT_1,
    TIMER_DIGIT_2,
    TIMER_DIGIT_3,
    TIMER_DIGIT_4,
    TIMER_DIGIT_5,
    TIMER_DIGIT_6,
    TIMER_DIGIT_7,
    TIMER_DIGIT_8,
    TIMER_DIGIT_9,
};

static const Timer_Glyph timer_startup_segments[] = {
    TIMER_SEGMENT_F,
    TIMER_SEGMENT_A,
    TIMER_SEGMENT_B,
    TIMER_SEGMENT_G,
    TIMER_SEGMENT_E,
    TIMER_SEGMENT_D,
    TIMER_SEGMENT_C,
    TIMER_SEGMENT_G,
};

static const uint16_t strikes_startup_segments[] = {
    STRIKES_SEGMENT_H,
    STRIKES_SEGMENT_J,
    STRIKES_SEGMENT_K,
    STRIKES_SEGMENT_G2,
    STRIKES_SEGMENT_L,
    STRIKES_SEGMENT_M,
    STRIKES_SEGMENT_N,
    STRIKES_SEGMENT_G1,
};

static void timer_display_set_led(const uint8_t index, const bool on) {
    const ARGB_LED colour = on ? timer->display_colour : (ARGB_LED){0};
    ARGB_Set(&timer->strip, index, colour.r, colour.g, colour.b);
}

static void timer_display_set_digit(const uint8_t offset, const Timer_Glyph glyph) {
    for (uint8_t led = 0; led < TIMER_LEDS_PER_DIGIT; led++) {
        timer_display_set_led(offset + led, (glyph & timer_digit_led_segments[led]) != 0);
    }
}

static void timer_display_set_all_digits(const Timer_Glyph glyph) {
    timer_display_set_digit(TIMER_DIGIT_1_LED_OFFSET, glyph);
    timer_display_set_digit(TIMER_DIGIT_2_LED_OFFSET, glyph);
    timer_display_set_digit(TIMER_DIGIT_3_LED_OFFSET, glyph);
    timer_display_set_digit(TIMER_DIGIT_4_LED_OFFSET, glyph);
}

static void timer_display_set_time(const uint8_t minutes,
                                   const uint8_t seconds,
                                   const uint8_t subseconds) {
    const bool show_minutes = minutes > 0;
    const uint8_t left_value = (show_minutes ? minutes : seconds) % 100;
    const uint8_t right_value = (show_minutes ? seconds : subseconds) % 100;

    timer_display_set_digit(TIMER_DIGIT_1_LED_OFFSET, timer_digit_glyphs[left_value / 10]);
    timer_display_set_digit(TIMER_DIGIT_2_LED_OFFSET, timer_digit_glyphs[left_value % 10]);
    timer_display_set_digit(TIMER_DIGIT_3_LED_OFFSET, timer_digit_glyphs[right_value / 10]);
    timer_display_set_digit(TIMER_DIGIT_4_LED_OFFSET, timer_digit_glyphs[right_value % 10]);

    timer_display_set_led(TIMER_DECIMAL_POINT_LED_OFFSET, !show_minutes);
    timer_display_set_led(TIMER_COLON_LOWER_LED_OFFSET, show_minutes);
    timer_display_set_led(TIMER_COLON_UPPER_LED_OFFSET, show_minutes);
}

static void timer_strikes_set(const uint16_t first, const uint16_t second) {
    timer->strikes_data = ((uint32_t) first << 16u) | second;
    SPI_Queue(&timer->strikes_transaction);
}

static bool timer_time_reached(const uint32_t now_ms, const uint32_t target_ms) {
    return (int32_t) (now_ms - target_ms) >= 0;
}

static uint32_t timer_elapsed_periods(const uint32_t now_ms,
                                      uint32_t *const next_ms,
                                      const uint32_t period_ms) {
    if (!timer_time_reached(now_ms, *next_ms)) {
        return 0;
    }

    const uint32_t elapsed_periods = ((now_ms - *next_ms) / period_ms) + 1u;
    *next_ms += elapsed_periods * period_ms;
    return elapsed_periods;
}

static void timer_init_enter(FSM *fsm) {
    timer->strip = (ARGB_Strip){
        .count = TIMER_LED_COUNT,
        .colour_order = COLOUR_ORDER_GRB,
        .max_brightness = 0x7f,
        .leds = timer->leds,
    };

    timer->display_colour = (ARGB_LED){
        .r = 0xff,
        .g = 0x00,
        .b = 0x00,
    };

    ARGB_Add_Strip(&timer->strip);

    timer->rotary_config = (IM_RotaryEncoderConfig){
        .slot = IM_ROTARY_SLOT_MODULE_A,
        .queue = &timer->input_queue,
        .event_mask = IM_EVENT_ROTARY_DELTA,
        .state = &timer->rotary_state,
        .invert_direction = false,
        .enable_internal_pullups = true,
        .counts_per_detent = TIMER_ROTARY_COUNTS_PER_DETENT,
    };

    IM_RegisterRotaryEncoder(&timer->rotary_config);

    timer->rotary_button_state = (IM_DigitalInputState){
        .channels = timer->rotary_button_channel_state,
    };

    timer->rotary_button_config = (IM_DigitalInputConfig){
        .rows = &GPIO_A_Pins[5],
        .row_count = 1,
        .queue = &timer->input_queue,
        .event_mask = IM_EVENT_DOWN | IM_EVENT_UP,
        .state = &timer->rotary_button_state,
        .scan_period_ms = 10,
        .debounce_ms = 30,
        .enable_internal_pullups = true,
        .active_high = false,
    };

    timer->rotary_button_handle = IM_RegisterDigital(&timer->rotary_button_config);

    GPIO_InitTypeDef gpio_init = {0};

    HAL_GPIO_WritePin(GPIO_A0_Port, GPIO_A0_Pin, GPIO_PIN_RESET);

    gpio_init.Pin = GPIO_A0_Pin;
    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIO_A0_Port, &gpio_init);

    HAL_GPIO_WritePin(GPIO_A1_Port, GPIO_A1_Pin, GPIO_PIN_RESET);

    gpio_init.Pin = GPIO_A1_Pin;
    HAL_GPIO_Init(GPIO_A1_Port, &gpio_init);

    /* Pull A7 low to provide ground for button and rotary encoder. */
    HAL_GPIO_WritePin(GPIO_A7_Port, GPIO_A7_Pin, GPIO_PIN_RESET);

    gpio_init.Pin = GPIO_A7_Pin;
    gpio_init.Mode = GPIO_MODE_OUTPUT_OD;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIO_A7_Port, &gpio_init);

    timer->strikes_transaction = (SPI_Transaction){
        .bits = 8,
        .baud = SPI_BAUD_125KHZ,
        .operation = SPI_OPERATION_WRITE,
        .cs_port = GPIO_A1_Port,
        .cs_pin = GPIO_A1_Pin,
        .cs_hold = false,
        .lsb_first = false,
        .cke = false,
        .ckp = false,
        .tx_data = &timer->strikes_data,
        .tx_size = sizeof(timer->strikes_data),
    };

    timer_strikes_set(STRIKES_DIGIT_BLANK, STRIKES_DIGIT_BLANK);

    timer_display_set_all_digits(TIMER_DIGIT_BLANK);
    timer_display_set_led(TIMER_DECIMAL_POINT_LED_OFFSET, false);
    timer_display_set_led(TIMER_COLON_LOWER_LED_OFFSET, false);
    timer_display_set_led(TIMER_COLON_UPPER_LED_OFFSET, false);

    FSM_Transition(fsm, MODE_FSM_STATE_STARTUP);
}

static void timer_startup_enter(FSM *fsm) {
    (void) fsm;

    timer->startup_step = 0;
    timer->startup_next_step_ms = HAL_GetTick() + TIMER_STARTUP_DELAY_MS;
}

static void timer_startup_service(FSM *fsm) {
    const uint32_t now_ms = HAL_GetTick();
    if (!timer_time_reached(now_ms, timer->startup_next_step_ms)) {
        return;
    }

    const uint8_t step_count = ARRAY_COUNT(timer_startup_segments) * TIMER_STARTUP_SEGMENT_REPEATS;
    if (timer->startup_step >= step_count) {
        FSM_Transition(fsm, MODE_FSM_STATE_IDLE);
        return;
    }

    const uint8_t step = timer->startup_step++;
    const Timer_Glyph timer_segment = timer_startup_segments[step % ARRAY_COUNT(timer_startup_segments)];
    const uint16_t strikes_segment = strikes_startup_segments[step % ARRAY_COUNT(strikes_startup_segments)];
    timer_display_set_all_digits(timer_segment);
    timer_strikes_set(strikes_segment, strikes_segment);

    timer->startup_next_step_ms = now_ms + TIMER_STARTUP_SEGMENT_MS;
}

static void timer_idle_enter(FSM *fsm) {
    timer_strikes_set(STRIKES_DIGIT_BLANK, STRIKES_DIGIT_BLANK);
    timer_display_set_all_digits(TIMER_DIGIT_DASH);
}

static void timer_attract_countdown_render(void) {
    const uint16_t total_centiseconds = timer->attract_remaining_centiseconds;
    const uint8_t minutes = total_centiseconds / 6000u;
    const uint8_t seconds = (total_centiseconds / 100u) % 60u;
    const uint8_t centiseconds = total_centiseconds % 100u;

    timer_display_set_time(minutes, seconds, centiseconds);
}

static void timer_attract_strikes_schedule_next_event(const uint32_t now_ms) {
    timer->attract_strikes_next_event_ms =
            now_ms + TRNG_Rand32Range(TIMER_ATTRACT_STRIKES_MIN_DELAY_MS,
                                      TIMER_ATTRACT_STRIKES_MAX_DELAY_MS);
}

static void timer_attract_strikes_set(const bool first, const bool second) {
    timer_strikes_set(first ? STRIKES_DIGIT_ASTERISK : STRIKES_DIGIT_BLANK,
                      second ? STRIKES_DIGIT_ASTERISK : STRIKES_DIGIT_BLANK);
}

static void timer_attract_strikes_start(const uint32_t now_ms) {
    timer_attract_strikes_set(false, false);
    timer->attract_strikes_state = TIMER_ATTRACT_STRIKES_WAIT_FIRST;
    timer->attract_strikes_visible = false;
    timer_attract_strikes_schedule_next_event(now_ms);
}

static void timer_attract_strikes_service(const uint32_t now_ms) {
    if (timer_time_reached(now_ms, timer->attract_strikes_next_event_ms)) {
        switch (timer->attract_strikes_state) {
            case TIMER_ATTRACT_STRIKES_WAIT_FIRST:
                timer_attract_strikes_set(true, false);
                timer->attract_strikes_state = TIMER_ATTRACT_STRIKES_WAIT_SECOND;
                timer_attract_strikes_schedule_next_event(now_ms);
                break;

            case TIMER_ATTRACT_STRIKES_WAIT_SECOND:
                timer_attract_strikes_set(true, true);
                timer->attract_strikes_state = TIMER_ATTRACT_STRIKES_FLASHING;
                timer->attract_strikes_visible = true;
                timer->attract_strikes_next_flash_ms =
                        now_ms + TIMER_ATTRACT_STRIKES_FLASH_HALF_PERIOD_MS;
                timer_attract_strikes_schedule_next_event(now_ms);
                break;

            case TIMER_ATTRACT_STRIKES_FLASHING:
            default:
                timer_attract_strikes_start(now_ms);
                return;
        }
    }

    if (timer->attract_strikes_state != TIMER_ATTRACT_STRIKES_FLASHING) {
        return;
    }

    const uint32_t elapsed_half_periods = timer_elapsed_periods(
        now_ms,
        &timer->attract_strikes_next_flash_ms,
        TIMER_ATTRACT_STRIKES_FLASH_HALF_PERIOD_MS);
    if ((elapsed_half_periods & 1u) != 0u) {
        timer->attract_strikes_visible = !timer->attract_strikes_visible;
        timer_attract_strikes_set(timer->attract_strikes_visible, timer->attract_strikes_visible);
    }
}

static void timer_attract_countdown_start(const uint32_t now_ms) {
    const uint16_t seconds = TRNG_Rand32Range(TIMER_ATTRACT_MIN_SECONDS,
                                              TIMER_ATTRACT_MAX_SECONDS);

    timer->attract_remaining_centiseconds = seconds * 100u;
    timer->attract_next_tick_ms = now_ms + TIMER_ATTRACT_TICK_MS;
    timer_attract_countdown_render();
}

static void timer_attract_enter(FSM *fsm) {
    const uint32_t now_ms = HAL_GetTick();
    timer_attract_countdown_start(now_ms);
    timer_attract_strikes_start(now_ms);
}

static void timer_attract_service(FSM *fsm) {
    const uint32_t now_ms = HAL_GetTick();
    timer_attract_strikes_service(now_ms);

    const uint32_t elapsed_ticks = timer_elapsed_periods(
        now_ms,
        &timer->attract_next_tick_ms,
        TIMER_ATTRACT_TICK_MS);
    if (elapsed_ticks == 0) {
        return;
    }

    if (timer->attract_remaining_centiseconds == 0) {
        timer_attract_countdown_start(now_ms);
        return;
    }

    if (elapsed_ticks >= timer->attract_remaining_centiseconds) {
        timer->attract_remaining_centiseconds = 0;
    } else {
        timer->attract_remaining_centiseconds -= elapsed_ticks;
    }

    timer_attract_countdown_render();
}

static Callbacks timer_state_callbacks[MODE_FSM_STATE_COUNT] = {
    [MODE_FSM_STATE_INIT] = {
        .enter = timer_init_enter,
    },
    [MODE_FSM_STATE_STARTUP] = {
        .enter = timer_startup_enter,
        .service = timer_startup_service,
    },
    [MODE_FSM_STATE_IDLE] = {
        .enter = timer_idle_enter,
    },
    [MODE_FSM_STATE_ATTRACT] = {
        .enter = timer_attract_enter,
        .service = timer_attract_service,
    },
    [MODE_FSM_STATE_PREPARE] = {0},
    [MODE_FSM_STATE_READY] = {0},
    [MODE_FSM_STATE_STARTING] = {0},
    [MODE_FSM_STATE_RUNNING] = {0},
    [MODE_FSM_STATE_SOLVED] = {0},
    [MODE_FSM_STATE_ENDED] = {0},
};

Mode_Definition timer_mode = {
    .state_callbacks = timer_state_callbacks,
    .always_service = NULL,
};
