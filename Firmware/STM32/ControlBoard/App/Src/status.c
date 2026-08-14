#include "status.h"

#include <stdbool.h>
#include <stdint.h>

#include "mode.h"
#include "stm32h5xx_hal.h"
#include "sys/gpio.h"
#include "sys/input_manager.h"

#define HEARTBEAT_HALF_PERIOD_MS 250u
#define MENU_ENTER_HOLD_MS       1000u
#define MENU_CONFIRM_HOLD_MS     2000u
#define MENU_IDLE_TIMEOUT_MS     10000u
#define MENU_FLASH_HALF_PERIOD_MS 250u
#define MENU_FLASH_GAP_MS        500u
#define MENU_OPTION_COUNT        3u

typedef enum {
    STATUS_HEARTBEAT,
    STATUS_MENU,
} Status_State;

typedef struct {
    Status_State state;
    uint32_t next_led_change_ms;
    uint32_t last_activity_ms;
    uint8_t option;
    uint8_t flashes_completed;
    bool led_on;
} Status;

static IM_EventQueue button_queue = {0};
static IM_DigitalChannelState button_channel_state[1] = {0};
static IM_DigitalInputState button_input_state = {0};
static IM_DigitalInputConfig button_input_config = {0};
static IM_Handle button_handle = IM_INVALID_HANDLE;
static Status status = {0};

static bool time_reached(const uint32_t now_ms, const uint32_t target_ms) {
    return (int32_t) (now_ms - target_ms) >= 0;
}

static bool time_elapsed(const uint32_t now_ms, const uint32_t since_ms, const uint32_t duration_ms) {
    return (uint32_t) (now_ms - since_ms) >= duration_ms;
}

static void led_set(const bool on) {
    status.led_on = on;
    HAL_GPIO_WritePin(STATUS_Port, STATUS_Pin, on ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

static void heartbeat_enter(const uint32_t now_ms) {
    status.state = STATUS_HEARTBEAT;
    led_set(false);
    status.next_led_change_ms = now_ms + HEARTBEAT_HALF_PERIOD_MS;
}

static void menu_enter(const uint32_t now_ms) {
    status.state = STATUS_MENU;
    status.last_activity_ms = now_ms;
    status.option = 1;
    status.flashes_completed = 0;
    led_set(false);
    status.next_led_change_ms = now_ms;
}

static void selected_mode_set(const uint32_t now_ms) {
    static const uint8_t modes[MENU_OPTION_COUNT] = {
        MODE_NONE,
        MODE_SUPPORT_CHASSIS,
        MODE_SUPPORT_TIMER,
    };

    Mode_Set(modes[status.option - 1u]);

    /* Mode_Set resets the MCU after a successful write. */
    heartbeat_enter(now_ms);
}

static void heartbeat_service(const uint32_t now_ms) {
    if (time_reached(now_ms, status.next_led_change_ms)) {
        led_set(!status.led_on);
        status.next_led_change_ms = now_ms + HEARTBEAT_HALF_PERIOD_MS;
    }
}

static void menu_led_service(const uint32_t now_ms) {
    if (!time_reached(now_ms, status.next_led_change_ms)) {
        return;
    }

    if (!status.led_on) {
        led_set(true);
        status.next_led_change_ms = now_ms + MENU_FLASH_HALF_PERIOD_MS;
        return;
    }

    led_set(false);
    status.flashes_completed++;

    if (status.flashes_completed >= status.option) {
        status.flashes_completed = 0;
        status.next_led_change_ms = now_ms + MENU_FLASH_GAP_MS;
    } else {
        status.next_led_change_ms = now_ms + MENU_FLASH_HALF_PERIOD_MS;
    }
}

static void menu_event_handle(const IM_Event *event) {
    status.last_activity_ms = event->timestamp_ms;

    if (event->event == IM_EVENT_DOWN) {
        return;
    }

    if (event->event != IM_EVENT_UP) {
        return;
    }

    if (event->duration_ms >= MENU_CONFIRM_HOLD_MS) {
        selected_mode_set(event->timestamp_ms);
        return;
    }

    if (event->duration_ms < MENU_ENTER_HOLD_MS) {
        status.option = (status.option % MENU_OPTION_COUNT) + 1u;
        status.flashes_completed = 0;
        led_set(false);
        status.next_led_change_ms = event->timestamp_ms;
    }
}

void Status_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* The open-drain status LED is active low. */
    HAL_GPIO_WritePin(STATUS_Port, STATUS_Pin, GPIO_PIN_SET);

    GPIO_InitStruct.Pin = STATUS_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(STATUS_Port, &GPIO_InitStruct);

    button_input_config = (IM_DigitalInputConfig) {
        .rows = &GPIO_Button_Pin,
        .row_count = 1,
        .queue = &button_queue,
        .event_mask = IM_EVENT_DOWN | IM_EVENT_UP,
        .state = &button_input_state,
        .scan_period_ms = 10,
        .debounce_ms = 30,
        .enable_internal_pullups = false,
        .active_high = true,
    };

    button_input_state = (IM_DigitalInputState) {
        .channels = button_channel_state,
    };

    button_handle = IM_RegisterDigital(&button_input_config);
    heartbeat_enter(HAL_GetTick());
}

void Status_Service(void) {
    const uint32_t now_ms = HAL_GetTick();
    IM_Event event;

    while (IM_EventQueue_Read(&button_queue, &event)) {
        if (event.handle != button_handle) {
            continue;
        }

        if (status.state == STATUS_HEARTBEAT) {
            if ((event.event == IM_EVENT_UP) && (event.duration_ms >= MENU_ENTER_HOLD_MS)) {
                menu_enter(event.timestamp_ms);
            }
        } else {
            menu_event_handle(&event);
        }
    }

    if (status.state == STATUS_HEARTBEAT) {
        heartbeat_service(now_ms);
        return;
    }

    if (time_elapsed(now_ms, status.last_activity_ms, MENU_IDLE_TIMEOUT_MS)) {
        heartbeat_enter(now_ms);
        return;
    }

    menu_led_service(now_ms);
}
