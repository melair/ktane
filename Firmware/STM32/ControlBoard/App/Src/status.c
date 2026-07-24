#include "status.h"
#include "gpio.h"
#include "stm32h5xx_hal.h"

void Status_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Default value to high, LED off */
    HAL_GPIO_WritePin(STATUS_Port, STATUS_Pin, GPIO_PIN_SET);

    /* Configure LED */
    GPIO_InitStruct.Pin = STATUS_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(STATUS_Port, &GPIO_InitStruct);

    /* Configure Button */
    GPIO_InitStruct.Pin = BUTTON_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BUTTON_Port, &GPIO_InitStruct);
}

void Status_Service(void) {

}
