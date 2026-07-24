#include "main.h"

#include "argb.h"
#include "can.h"
#include "gpio.h"
#include "i2c.h"
#include "mcu_init.h"
#include "rng.h"
#include "spi.h"
#include "status.h"

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void) {
    /* Configure MPU */
    MPU_Init();

    /* Reset of all peripherals, initializes the Flash interface and the Systick */
    HAL_Init();

    /* Configure the system clock */
    SystemClock_Init();

    /* Configure PLL2 and PLL3 ahead of time */
    PeripheralClock_Init();

    /* Initialize base MCU peripherals */
    GPIO_Init();
    ICACHE_Init();
    RTC_Init();
    TRNG_Init();

    /* Enable DMA peripherals. GPDMA1 for core functions, GPDMA2 for submodules. */
    __HAL_RCC_GPDMA1_CLK_ENABLE();
    __HAL_RCC_GPDMA2_CLK_ENABLE();

    /* Initialize common controlboard processes and peripherals. */
    Status_Init();
    I2C_Init();
    SPI_Init();
    CAN_Init();
    ARGB_Init();
    // TODO: CBUS_Init();
    // TODO: SDMMC_Init();
    // TODO: USB_Init();

    /* Infinite loop */
    while (1) {
        /* Service the status LED, and button. */
        Status_Service();
        /* Service SPI. */
        SPI_Service();
        /* Service I2C. */
        I2C_Service();
    }
}
