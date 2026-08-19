#include "main.h"

#include "i2c.h"
#include "input_manager.h"
#include "status.h"
#include "sys/gpio.h"
#include "sys/mcu_init.h"

static void status_menu_selection(uint8_t value) {
    (void) value;
}

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void) {
    /* Reset of all peripherals, initializes the Flash interface and the Systick */
    HAL_Init();

    /* Configure the system clock */
    SystemClock_Init();

    /* Initialize base MCU peripherals */
    GPIO_Init();

    /* Enable DMA peripherals. DMA1 for all functions. */
    __HAL_RCC_DMA1_CLK_ENABLE();

    /* Initialize common backplane processes and peripherals. */
    IM_Init();
    if (!I2C_Init()) {
        Error_Handler();
    }

    /* Base service init. */
    if (!Status_Init((GPIO_PinDef) {STATUS_LED_GPIO_Port, STATUS_LED_Pin},
                     (GPIO_PinDef) {BUTTON_GPIO_Port, BUTTON_Pin},
                     false,
                     7U,
                     status_menu_selection)) {
        Error_Handler();
    }

    /* Infinite loop */
    while (1) {
        /* Service input manager. */
        IM_Service();
        /* Service the status LED and button. */
        Status_Service();
        /* Service I2C. */
        I2C_Service();

        __WFI();
    }
}
