#include "i2c/i2c_platform.h"

static I2C_HandleTypeDef i2c_handle;
static DMA_HandleTypeDef i2c_dma_handle;

static bool configure_clock(void) {
    RCC_PeriphCLKInitTypeDef clock_config = {0};

    clock_config.PeriphClockSelection = RCC_PERIPHCLK_I2C1;
    clock_config.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1;
    return HAL_RCCEx_PeriphCLKConfig(&clock_config) == HAL_OK;
}

static void enable_peripheral_clock(void) {
    __HAL_RCC_I2C1_CLK_ENABLE();
}

const I2C_Hardware I2C_HARDWARE = {
    .i2c_instance = I2C1,
    .i2c_handle = &i2c_handle,
    .scl = {I2C_SCL_Port, I2C_SCL_Pin},
    .scl_alternate = GPIO_AF6_I2C1,
    .sda = {I2C_SDA_Port, I2C_SDA_Pin},
    .sda_alternate = GPIO_AF6_I2C1,
    .timing = 0x10B17DB5,
    .dma_instance = DMA1_Channel1,
    .dma_handle = &i2c_dma_handle,
    .dma_tx_request = DMA_REQUEST_I2C1_TX,
    .dma_rx_request = DMA_REQUEST_I2C1_RX,
    .dma_irq = DMA1_Channel1_IRQn,
    .event_irq = I2C1_IRQn,
    .error_irq = I2C1_IRQn,
    .configure_clock = configure_clock,
    .enable_peripheral_clock = enable_peripheral_clock,
};

void DMA1_Channel1_IRQHandler(void) {
    I2C_Platform_DMA_IRQHandler();
}

void I2C1_IRQHandler(void) {
    I2C_Platform_Event_IRQHandler();
    I2C_Platform_Error_IRQHandler();
}
