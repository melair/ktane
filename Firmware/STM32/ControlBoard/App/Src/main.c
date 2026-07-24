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
    //TRNG_Init();

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


    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_C0_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIO_C0_Port, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIO_C0_Port, GPIO_C0_Pin, GPIO_PIN_SET);

    uint32_t next = 0;

    SPI_Transaction t = {0};
    uint8_t data[8] = {0xaa, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x55};

    /* Infinite loop */
    while (1) {
        /* Service the status LED, and button. */
        Status_Service();
        /* Service SPI. */
        SPI_Service();

        if (uwTick > next) {
            next = uwTick + 1000;

            t.baud = SPI_BAUD_8MHZ;
            t.bits = 8;
            t.cs_pin = GPIO_C0_Pin;
            t.cs_port = GPIO_C0_Port;
            t.operation = SPI_OPERATION_WRITE;
            t.tx_size = 8;
            t.tx_data = &data;

            SPI_Queue(&t);
        }

    }
}
