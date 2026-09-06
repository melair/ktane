#include "main.h"

#include "backplane.h"
#include "chassis_bus.h"
#include "i2c/i2c.h"
#include "input_manager/input_manager.h"
#include "node_link.h"
#include "power.h"
#include "status/status.h"
#include "sys/gpio.h"
#include "sys/mcu_init.h"
#include "nvm/nvm.h"

static const BackplaneLocation status_menu_locations[] = {
    BACKPLANE_LOCATION_0,
    BACKPLANE_LOCATION_1,
    BACKPLANE_LOCATION_2,
    BACKPLANE_LOCATION_3,
    BACKPLANE_LOCATION_4,
    BACKPLANE_LOCATION_5,
    BACKPLANE_LOCATION_CHASSIS,
};

#define STATUS_MENU_LOCATION_COUNT \
    ((uint8_t) (sizeof(status_menu_locations) / sizeof(status_menu_locations[0])))

_Static_assert(STATUS_MENU_LOCATION_COUNT == 7U,
               "Status menu must contain all backplane locations");

static void status_menu_selection(uint8_t value) {
    if ((value == 0U) || (value > STATUS_MENU_LOCATION_COUNT)) {
        return;
    }

    if (Backplane_SetLocation(status_menu_locations[value - 1U])) {
        NVIC_SystemReset();
    }
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

    /* Initialize the flash-backed non-volatile journal. */
    NVM_Init();

    /* Load the backplane location before initializing operational peripherals. */
    if (!Backplane_Init()) {
        Error_Handler();
    }

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
    if (!NodeLink_Init()) {
        Error_Handler();
    }
    if (!Power_Init()) {
        Error_Handler();
    }

    /* Base service init. */
    if (!Status_Init((GPIO_PinDef) {STATUS_LED_GPIO_Port, STATUS_LED_Pin},
                     (GPIO_PinDef) {BUTTON_GPIO_Port, BUTTON_Pin},
                     false,
                     STATUS_MENU_LOCATION_COUNT,
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
        /* Service the front and rear node links. */
        NodeLink_Service();
        /* Service front and rear module power. */
        Power_Service();

        __WFI();
    }
}
