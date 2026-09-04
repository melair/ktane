#include "main.h"

#include "chassis_bus.h"
#include "game.h"
#include "indicator.h"
#include "mode.h"
#include "nodes.h"
#include "protocol.h"
#include "sys/argb.h"
#include "sys/can.h"
#include "sys/gpio.h"
#include "i2c.h"
#include "input_manager.h"
#include "sys/mcu_init.h"
#include "sys/mcu_load.h"
#include "sys/nvm.h"
#include "sys/rng.h"
#include "sys/rtc.h"
#include "spi.h"
#include "sys/sys_clock.h"
#include "status.h"
#include "sys/tick.h"
#include "stm32h5xx_it.h"

static const uint8_t status_menu_modes[] = {
    MODE_NONE,
    MODE_SUPPORT_CHASSIS,
    MODE_SUPPORT_TIMER,
};

static void status_menu_selection(uint8_t value) {
    const uint8_t menu_value_count = sizeof(status_menu_modes) / sizeof(status_menu_modes[0]);

    if ((value == 0U) || (value > menu_value_count)) {
        return;
    }

    Mode_Set(status_menu_modes[value - 1U]);
}

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

    /* Read and fold the MCU unique identifier. */
    UID_Init();

    /* Configure the non-volatile high-cycle data area */
    NVM_Init();

    /* Configure PLL2 and PLL3 ahead of time */
    PeripheralClock_Init();

    /* Initialize base MCU peripherals */
    GPIO_Init();
    ICACHE_Init();
    RTC_Init();
    TRNG_Init();
    SysClock_Init();

    /* Enable DMA peripherals. GPDMA1 for core functions, GPDMA2 for submodules. */
    __HAL_RCC_GPDMA1_CLK_ENABLE();
    __HAL_RCC_GPDMA2_CLK_ENABLE();

    /* Initialize common controlboard processes and peripherals. */
    IM_Init();
    if (!I2C_Init()) {
        Error_Handler();
    }
    if (!SPI_Init()) {
        Error_Handler();
    }
    CAN_Init();
    Protocol_Init();
    ARGB_Init();
    if (!ChassisBus_Init()) {
        Error_Handler();
    }

    /* Base service init. */
    if (!Status_Init((GPIO_PinDef) {STATUS_Port, STATUS_Pin},
                     (GPIO_PinDef) {BUTTON_Port, BUTTON_Pin},
                     true,
                     sizeof(status_menu_modes) / sizeof(status_menu_modes[0]),
                     status_menu_selection)) {
        Error_Handler();
    }

    /* Initialize MCU load timing. */
    MCU_Load_Init();

    /* Initialize periodic tick flags. */
    Tick_Init();

    /* Initialize the module indicator to OFF. */
    Indicator_Init();

    /* Initialize the game. */
    Game_Init();

    /* Initialize the selected mode. */
    Mode_Init();

    /* Infinite loop */
    while (1) {
        /* Update periodic tick flags for this service pass. */
        Tick_Service_Start();

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
        CAN_Service(Protocol_Receive);
        /* Update node activity. */
        Nodes_Service();
        /* Service protocol. */
        Protocol_Service();
        /* Service the chassis bus. */
        ChassisBus_Service();

        /* Service the game. */
        Game_Service();
        /* Service the mode. */
        Mode_Service();

        /* Service the module indicator. */
        Indicator_Service();
        /* Service ARGB strips with pending changes. */
        ARGB_Service();

        /* Account time spent. */
        MCU_Load_End();

        /* Clear tick flags and only wait if processing did not cross an ms boundary. */
        if (Tick_Service_End()) {
            __WFI();
        }
    }
}
