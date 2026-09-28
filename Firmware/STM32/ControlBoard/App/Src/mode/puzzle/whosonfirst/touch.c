#include "mode/puzzle/whosonfirst/touch.h"

#include <string.h>

#define TOUCH_RESET_ASSERT_MS 10U
#define TOUCH_RESET_READY_MS 300U

#define TOUCH_REGISTER_TD_STATUS 0x02U
#define TOUCH_EVENT_MASK 0xC0U
#define TOUCH_EVENT_DOWN 0x00U
#define TOUCH_EVENT_UP 0x40U
#define TOUCH_EVENT_CONTACT 0x80U

typedef enum {
    TOUCH_STATE_RESET_ASSERT,
    TOUCH_STATE_RESET_RELEASE,
    TOUCH_STATE_WAIT_INTERRUPT,
    TOUCH_STATE_READ_REPORT,
} Touch_State;

static void reset_assert_enter(FSM *fsm);
static void reset_release_enter(FSM *fsm);
static void wait_interrupt_service(FSM *fsm);

static const FSM_State touch_states[] = {
    [TOUCH_STATE_RESET_ASSERT] = {
        .enter = reset_assert_enter,
        .next_mask = FSM_NEXT(TOUCH_STATE_RESET_RELEASE),
    },
    [TOUCH_STATE_RESET_RELEASE] = {
        .enter = reset_release_enter,
        .next_mask = FSM_NEXT(TOUCH_STATE_WAIT_INTERRUPT),
    },
    [TOUCH_STATE_WAIT_INTERRUPT] = {
        .service = wait_interrupt_service,
        .next_mask = FSM_NEXT(TOUCH_STATE_READ_REPORT),
    },
    [TOUCH_STATE_READ_REPORT] = {
        .next_mask = FSM_NEXT(TOUCH_STATE_WAIT_INTERRUPT),
    },
};

static I2C_Transaction *transfer_complete(I2C_Transaction *transaction) {
    Touch *touch = transaction->callback_data;
    if (touch == NULL) {
        return NULL;
    }

    (void) FSM_Transition(&touch->fsm, TOUCH_STATE_WAIT_INTERRUPT);
    if (transaction->status != I2C_STATUS_SUCCESS) {
        return NULL;
    }

    const uint8_t touch_count = touch->report[0] & 0x0FU;
    const uint8_t event = touch->report[1] & TOUCH_EVENT_MASK;
    if (touch_count == 0U || event == TOUCH_EVENT_UP) {
        touch->touch_active = false;
        return NULL;
    }

    if (event != TOUCH_EVENT_DOWN && event != TOUCH_EVENT_CONTACT) {
        return NULL;
    }

    const uint16_t x = ((uint16_t) (touch->report[1] & 0x0FU) << 8U) | touch->report[2];
    const uint16_t y = ((uint16_t) (touch->report[3] & 0x0FU) << 8U) | touch->report[4];
    if (touch->touch_active && touch->last_x == x && touch->last_y == y) {
        return NULL;
    }

    touch->last_x = x;
    touch->last_y = y;
    touch->touch_active = true;
    const IM_Event input_event = {
        .handle = IM_INVALID_HANDLE,
        .channel = 0U,
        .event = IM_EVENT_TOUCH,
        .timestamp_ms = HAL_GetTick(),
        .duration_ms = 0U,
        .delta = 0,
        .value = 0U,
        .x = x,
        .y = y,
    };
    (void) IM_EventQueue_Write(touch->config.queue, &input_event);
    return NULL;
}

static void reset_assert_enter(FSM *fsm) {
    Touch *touch = fsm->context;
    HAL_GPIO_WritePin(touch->config.reset.port, touch->config.reset.pin, GPIO_PIN_RESET);
    (void) FSM_TransitionIn(fsm, TOUCH_STATE_RESET_RELEASE, TOUCH_RESET_ASSERT_MS);
}

static void reset_release_enter(FSM *fsm) {
    Touch *touch = fsm->context;
    HAL_GPIO_WritePin(touch->config.reset.port, touch->config.reset.pin, GPIO_PIN_SET);
    (void) FSM_TransitionIn(fsm, TOUCH_STATE_WAIT_INTERRUPT, TOUCH_RESET_READY_MS);
}

static void wait_interrupt_service(FSM *fsm) {
    Touch *touch = fsm->context;
    if (HAL_GPIO_ReadPin(touch->config.interrupt.port, touch->config.interrupt.pin) != GPIO_PIN_RESET) {
        return;
    }

    I2C_Queue(&touch->transaction);
    (void) FSM_Transition(fsm, TOUCH_STATE_READ_REPORT);
}

bool Touch_Init(Touch *touch, const Touch_Config *config) {
    if (touch == NULL || config == NULL || config->reset.port == NULL ||
        config->interrupt.port == NULL || config->queue == NULL) {
        return false;
    }

    const Touch_Config saved_config = *config;
    memset(touch, 0, sizeof(*touch));
    touch->config = saved_config;
    touch->register_address = TOUCH_REGISTER_TD_STATUS;
    touch->transaction = (I2C_Transaction) {
        .operation = I2C_OPERATION_WRITE_RESTART_READ,
        .address = touch->config.address,
        .tx_data = &touch->register_address,
        .tx_size = 1U,
        .rx_data = touch->report,
        .rx_size = sizeof(touch->report),
        .callback = transfer_complete,
        .callback_data = touch,
    };

    GPIO_InitTypeDef gpio = {
        .Mode = GPIO_MODE_OUTPUT_PP,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_LOW,
    };
    gpio.Pin = touch->config.reset.pin;
    HAL_GPIO_WritePin(touch->config.reset.port, gpio.Pin, GPIO_PIN_RESET);
    HAL_GPIO_Init(touch->config.reset.port, &gpio);

    gpio = (GPIO_InitTypeDef) {
        .Pin = touch->config.interrupt.pin,
        .Mode = GPIO_MODE_INPUT,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_LOW,
    };
    HAL_GPIO_Init(touch->config.interrupt.port, &gpio);

    touch->initialized = FSM_Init(&touch->fsm, touch_states, TOUCH_STATE_RESET_ASSERT, touch);
    return touch->initialized;
}

void Touch_Service(Touch *touch) {
    if (touch != NULL && touch->initialized) {
        FSM_Service(&touch->fsm);
    }
}
