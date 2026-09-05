#include "main.h"

#include "argb.h"
#include "i2c.h"
#include "input_manager.h"
#include "sys/gpio.h"
#include "sys/mcu_init.h"

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

    /* Initialize common edgework processes and peripherals. */
    IM_Init();
    if (!I2C_Init()) {
        Error_Handler();
    }
    if (!ARGB_Init()) {
        Error_Handler();
    }

    /* Infinite loop */
    while (1) {
        /* Service input manager. */
        IM_Service();
        /* Service I2C. */
        I2C_Service();
        /* Service ARGB strips with pending changes. */
        ARGB_Service();
        __WFI();
    }
}
