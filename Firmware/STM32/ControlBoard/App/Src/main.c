#include "main.h"

#include "sys/argb.h"
#include "sys/can.h"
#include "sys/cbus.h"
#include "sys/gpio.h"
#include "sys/i2c.h"
#include "sys/input_manager.h"
#include "sys/mcu_init.h"
#include "sys/mcu_load.h"
#include "sys/nvm.h"
#include "sys/rng.h"
#include "sys/rtc.h"
#include "sys/spi.h"
#include "status.h"
#include "sys/tick.h"

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

    /* Configure the non-volatile high-cycle data area */
    NVM_Init();

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
    IM_Init();
    I2C_Init();
    SPI_Init();
    CAN_Init();
    ARGB_Init();
    CBUS_Init();

    /* Base service init. */
    Status_Init();

    /* Initialize MCU load timing. */
    MCU_Load_Init();

    /* Initialize periodic tick flags. */
    tick_init();

    /* Infinite loop */
    while (1) {
        /* Update periodic tick flags for this service pass. */
        tick_service_start();

        /* Start accounting time. */
        MCU_Load_Begin();

        /* Service input manager. */
        IM_Service();
        /* Service the status LED, and button. */
        Status_Service();
        /* Service SPI. */
        SPI_Service();
        /* Service I2C. */
        I2C_Service();
        /* Service CAN. */
        CAN_Service(NULL);
        /* Service CBUS. */
        CBUS_Service(NULL);

        /* Account time spent. */
        MCU_Load_End();

        /* Clear tick flags and only wait if processing did not cross an ms boundary. */
        if (tick_service_end()) {
            __WFI();
        }
    }
}
