#include "main.h"

#include "chassis_bus.h"
#include "i2c.h"
#include "input_manager.h"
#include "module_link.h"
#include "power.h"
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

    /* Disable the UCPD1 dead-battery pull-downs on PA8 and PA9. */
    HAL_SYSCFG_StrobeDBattpinsConfig(SYSCFG_UCPD1_STROBE);

    /* Enable DMA peripherals. DMA1 for all functions. */
    __HAL_RCC_DMA1_CLK_ENABLE();

    /* Initialize common backplane processes and peripherals. */
    IM_Init();
    if (!I2C_Init()) {
        Error_Handler();
    }
    if (!ChassisBus_Init()) {
        Error_Handler();
    }
    if (!ModuleLink_Init()) {
        Error_Handler();
    }
    if (!Power_Init()) {
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
        /* Service the chassis bus. */
        ChassisBus_Service();
        /* Service the front and rear module links. */
        ModuleLink_Service();
        /* Service front and rear module power. */
        Power_Service();

        __WFI();
    }
}
