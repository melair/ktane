#include "main.h"

#include "argb/argb.h"
#include "edgework_bus.h"
#include "i2c/i2c.h"
#include "input_manager/input_manager.h"
#include "mode.h"
#include "slot.h"
#include "sys/gpio.h"
#include "sys/mcu_init.h"
#include "nvm/nvm.h"

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void) {
    /* Reset of all peripherals, initializes the Flash interface and the Systick */
    HAL_Init();

    /* Configure the system clock */
    SystemClock_Init();

    /* Initialize the flash-backed non-volatile journal. */
    NVM_Init();

    /* Initialize base MCU peripherals */
    GPIO_Init();

    /* Initialize common input handling before modes register their inputs. */
    IM_Init();

    /* Load the configured edgework mode after its dependencies are initialized. */
    if (!Mode_Init()) {
        Error_Handler();
    }

    /* Disable the UCPD1 dead-battery pull-downs on PA8 and PA9. */
    HAL_SYSCFG_StrobeDBattpinsConfig(SYSCFG_UCPD1_STROBE);

    /* Enable DMA peripherals. DMA1 for all functions. */
    __HAL_RCC_DMA1_CLK_ENABLE();

    /* Initialize common edgework processes and peripherals. */
    if (!I2C_Init()) {
        Error_Handler();
    }
    Slot_Init();
    if (!EdgeworkBus_Init()) {
        Error_Handler();
    }

    /* Infinite loop */
    while (1) {
        /* Service input manager. */
        IM_Service();
        /* Service I2C. */
        I2C_Service();
        /* Service the slot EEPROM state machine. */
        Slot_Service();
        /* Service the edgework mode state machine. */
        Mode_Service();
        /* Service the edgework bus. */
        EdgeworkBus_Service();
        /* Wait for next interrupt. */
        __WFI();
    }
}
