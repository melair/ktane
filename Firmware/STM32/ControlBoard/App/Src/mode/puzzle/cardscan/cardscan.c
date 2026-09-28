#include "mode/puzzle/cardscan/cardscan.h"
#include "mode.h"
#include "mode_fsm.h"
#include "sys/gpio.h"
#include <stddef.h>

#define LED_RED_Pin GPIO_A7_Pin
#define LED_RED_Port GPIO_A7_Port
#define LED_YELLOW_Pin GPIO_A6_Pin
#define LED_YELLOW_Port GPIO_A6_Port
#define LED_GREEN_Pin GPIO_A5_Pin
#define LED_GREEN_Port GPIO_A5_Port
#define NFC_IRQ_Pin GPIO_B7_Pin
#define NFC_IRQ_Port GPIO_B7_Port

#define CARDSCAN_STARTUP_STEP_MS 500U

static CardScan_Data *const cardscan = &mode_data.mode.cardscan;

static void cardscan_red_service(void) {
    if (cardscan->red_flash_active &&
        (int32_t) (HAL_GetTick() - cardscan->red_flash_until_ms) >= 0) {
        cardscan->red_flash_active = false;
    }
    HAL_GPIO_WritePin(LED_RED_Port, LED_RED_Pin,
                     cardscan->red_startup_on || cardscan->red_flash_active ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void cardscan_yellow_service(void) {
    if (cardscan->yellow_flash_active &&
        (int32_t) (HAL_GetTick() - cardscan->yellow_flash_until_ms) >= 0) {
        cardscan->yellow_flash_active = false;
    }
    HAL_GPIO_WritePin(LED_YELLOW_Port, LED_YELLOW_Pin,
                     cardscan->yellow_startup_on || cardscan->yellow_flash_active ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void cardscan_always_service(void) {
    uint8_t card_id;
    const PN532_ScanResult result = PN532_Service(&cardscan->nfc,
        HAL_GPIO_ReadPin(NFC_IRQ_Port, NFC_IRQ_Pin) == GPIO_PIN_RESET, &card_id);
    if (result == PN532_SCAN_VALID) {
        cardscan->scanned_id = card_id;
        cardscan->scanned_updated = true;
        cardscan->red_flash_active = true;
        cardscan->red_flash_until_ms = HAL_GetTick() + 250U;
    } else if (result == PN532_SCAN_INVALID) {
        cardscan->yellow_flash_active = true;
        cardscan->yellow_flash_until_ms = HAL_GetTick() + 250U;
    }
    cardscan_red_service();
    cardscan_yellow_service();
}

static void cardscan_init_enter(FSM *fsm) {
    GPIO_InitTypeDef gpio_init = {0};
    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;

    gpio_init.Pin = LED_RED_Pin;
    HAL_GPIO_Init(LED_RED_Port, &gpio_init);

    gpio_init.Pin = LED_YELLOW_Pin;
    HAL_GPIO_Init(LED_YELLOW_Port, &gpio_init);

    gpio_init.Pin = LED_GREEN_Pin;
    HAL_GPIO_Init(LED_GREEN_Port, &gpio_init);

    gpio_init.Pin = NFC_IRQ_Pin;
    gpio_init.Mode = GPIO_MODE_INPUT;
    gpio_init.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(NFC_IRQ_Port, &gpio_init);

    cardscan->scanned_id = 0;
    cardscan->scanned_updated = false;
    cardscan->red_flash_active = false;
    cardscan->yellow_flash_active = false;
    PN532_Init(&cardscan->nfc);
    Mode_SetServiceEnabled(true);

    FSM_Transition(fsm, MODE_FSM_STATE_STARTUP);
}

static void cardscan_startup_enter(FSM *fsm) {
    (void) fsm;

    cardscan->yellow_startup_on = false;
    cardscan_yellow_service();
    HAL_GPIO_WritePin(LED_GREEN_Port, LED_GREEN_Pin, GPIO_PIN_RESET);
    cardscan->red_startup_on = true;
    cardscan_red_service();

    cardscan->startup_step = 0;
    cardscan->startup_next_step_ms = HAL_GetTick() + CARDSCAN_STARTUP_STEP_MS;
}

static void cardscan_startup_service(FSM *fsm) {
    const uint32_t now_ms = HAL_GetTick();
    if ((int32_t) (now_ms - cardscan->startup_next_step_ms) < 0) {
        return;
    }

    switch (cardscan->startup_step++) {
        case 0:
            cardscan->yellow_startup_on = true;
            cardscan_yellow_service();
            break;
        case 1:
            HAL_GPIO_WritePin(LED_GREEN_Port, LED_GREEN_Pin, GPIO_PIN_SET);
            break;
        case 2:
            cardscan->red_startup_on = false;
            cardscan_red_service();
            break;
        case 3:
            cardscan->yellow_startup_on = false;
            cardscan_yellow_service();
            break;
        case 4:
            HAL_GPIO_WritePin(LED_GREEN_Port, LED_GREEN_Pin, GPIO_PIN_RESET);
            FSM_Transition(fsm, MODE_FSM_STATE_IDLE);
            return;
        default:
            return;
    }

    cardscan->startup_next_step_ms = now_ms + CARDSCAN_STARTUP_STEP_MS;
}

static Callbacks cardscan_state_callbacks[MODE_FSM_STATE_COUNT] = {
    [MODE_FSM_STATE_INIT] = {
        .enter = cardscan_init_enter,
    },
    [MODE_FSM_STATE_STARTUP] = {
        .enter = cardscan_startup_enter,
        .service = cardscan_startup_service,
    },
    [MODE_FSM_STATE_IDLE] = {0},
    [MODE_FSM_STATE_ATTRACT] = {0},
    [MODE_FSM_STATE_PREPARE] = {0},
    [MODE_FSM_STATE_READY] = {0},
    [MODE_FSM_STATE_STARTING] = {0},
    [MODE_FSM_STATE_RUNNING] = {0},
    [MODE_FSM_STATE_SOLVED] = {0},
    [MODE_FSM_STATE_ENDED] = {0},
};

Mode_Definition cardscan_mode = {
    .state_callbacks = cardscan_state_callbacks,
    .always_service = cardscan_always_service,
};
