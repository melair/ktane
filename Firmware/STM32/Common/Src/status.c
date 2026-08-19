#include "status.h"

#include "input_manager.h"

#define HEARTBEAT_HALF_PERIOD_MS 500U
#define MENU_ENTER_HOLD_MS 1000U
#define MENU_CONFIRM_HOLD_MS 2000U
#define MENU_IDLE_TIMEOUT_MS 10000U
#define MENU_FLASH_HALF_PERIOD_MS 250U
#define MENU_FLASH_GAP_MS 1000U

typedef enum {
    STATUS_HEARTBEAT,
    STATUS_MENU,
} Status_State;

typedef struct {
    Status_State state;
    GPIO_PinDef led_pin;
    GPIO_PinDef button_pin;
    Status_SelectionCallback selection_callback;
    uint32_t next_led_change_ms;
    uint32_t last_activity_ms;
    uint8_t max_menu_value;
    uint8_t selected_value;
    uint8_t flashes_completed;
    bool led_on;
} Status;

static IM_EventQueue button_queue = {0};
static IM_DigitalChannelState button_channel_state[1] = {0};
static IM_DigitalInputState button_input_state = {0};
static IM_DigitalInputConfig button_input_config = {0};
static IM_Handle button_handle = IM_INVALID_HANDLE;
static Status status = {0};

static bool time_reached(uint32_t now_ms, uint32_t target_ms) {
    return (int32_t) (now_ms - target_ms) >= 0;
}

static bool time_elapsed(uint32_t now_ms, uint32_t since_ms, uint32_t duration_ms) {
    return (uint32_t) (now_ms - since_ms) >= duration_ms;
}

static void led_set(bool on) {
    status.led_on = on;
    HAL_GPIO_WritePin(status.led_pin.port, status.led_pin.pin, on ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

static void heartbeat_enter(uint32_t now_ms) {
    status.state = STATUS_HEARTBEAT;
    led_set(false);
    status.next_led_change_ms = now_ms + HEARTBEAT_HALF_PERIOD_MS;
}

static void menu_enter(uint32_t now_ms) {
    status.state = STATUS_MENU;
    status.last_activity_ms = now_ms;
    status.selected_value = 1U;
    status.flashes_completed = 0;
    led_set(false);
    status.next_led_change_ms = now_ms;
}

static void selected_value_publish(uint32_t now_ms) {
    status.selection_callback(status.selected_value);
    heartbeat_enter(now_ms);
}

static void heartbeat_service(uint32_t now_ms) {
    if (time_reached(now_ms, status.next_led_change_ms)) {
        led_set(!status.led_on);
        status.next_led_change_ms = now_ms + HEARTBEAT_HALF_PERIOD_MS;
    }
}

static void menu_led_service(uint32_t now_ms) {
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

    if (status.flashes_completed >= status.selected_value) {
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
        selected_value_publish(event->timestamp_ms);
        return;
    }

    if (event->duration_ms < MENU_ENTER_HOLD_MS) {
        status.selected_value = (status.selected_value % status.max_menu_value) + 1U;
        status.flashes_completed = 0;
        led_set(false);
        status.next_led_change_ms = event->timestamp_ms;
    }
}

bool Status_Init(GPIO_PinDef led_pin, GPIO_PinDef button_pin, bool button_active_high, uint8_t max_menu_value,
                 Status_SelectionCallback selection_callback) {
    if ((led_pin.port == NULL) || (led_pin.pin == 0U) ||
        (button_pin.port == NULL) || (button_pin.pin == 0U) ||
        (max_menu_value == 0U) || (selection_callback == NULL)) {
        return false;
    }

    status = (Status) {
        .led_pin = led_pin,
        .button_pin = button_pin,
        .selection_callback = selection_callback,
        .max_menu_value = max_menu_value,
    };

    button_queue = (IM_EventQueue) {0};
    button_channel_state[0] = (IM_DigitalChannelState) {0};
    button_input_state = (IM_DigitalInputState) {
        .channels = button_channel_state,
    };
    button_input_config = (IM_DigitalInputConfig) {
        .rows = &status.button_pin,
        .row_count = 1,
        .queue = &button_queue,
        .event_mask = IM_EVENT_DOWN | IM_EVENT_UP,
        .state = &button_input_state,
        .scan_period_ms = 10,
        .debounce_ms = 30,
        .enable_internal_pullups = false,
        .active_high = button_active_high,
    };

    GPIO_InitTypeDef gpio_init = {0};

    /* The open-drain status LED is active low. */
    HAL_GPIO_WritePin(status.led_pin.port, status.led_pin.pin, GPIO_PIN_SET);

    gpio_init.Pin = status.led_pin.pin;
    gpio_init.Mode = GPIO_MODE_OUTPUT_OD;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(status.led_pin.port, &gpio_init);

    button_handle = IM_RegisterDigital(&button_input_config);
    if (button_handle == IM_INVALID_HANDLE) {
        return false;
    }

    heartbeat_enter(HAL_GetTick());
    return true;
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
