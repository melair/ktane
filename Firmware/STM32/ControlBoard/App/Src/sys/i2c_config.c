#include "i2c/i2c_platform.h"

static I2C_HandleTypeDef i2c_handle;
static DMA_HandleTypeDef i2c_dma_handle;

static bool configure_clock(void) {
    __HAL_RCC_I2C2_CONFIG(RCC_I2C2CLKSOURCE_PCLK1);
    return true;
}

static void enable_peripheral_clock(void) {
    __HAL_RCC_I2C2_CLK_ENABLE();
}

const I2C_Hardware I2C_HARDWARE = {
    .i2c_instance = I2C2,
    .i2c_handle = &i2c_handle,
    .scl = {I2C_SCL_Port, I2C_SCL_Pin},
    .scl_alternate = GPIO_AF4_I2C2,
    .sda = {I2C_SDA_Port, I2C_SDA_Pin},
    .sda_alternate = GPIO_AF4_I2C2,
    .timing = 0x60808CD3,
    .dma_instance = GPDMA1_Channel2,
    .dma_handle = &i2c_dma_handle,
    .dma_tx_request = GPDMA1_REQUEST_I2C2_TX,
    .dma_rx_request = GPDMA1_REQUEST_I2C2_RX,
    .dma_irq = GPDMA1_Channel2_IRQn,
    .event_irq = I2C2_EV_IRQn,
    .error_irq = I2C2_ER_IRQn,
    .configure_clock = configure_clock,
    .enable_peripheral_clock = enable_peripheral_clock,
};

void GPDMA1_Channel2_IRQHandler(void) {
    I2C_Platform_DMA_IRQHandler();
}

void I2C2_EV_IRQHandler(void) {
    I2C_Platform_Event_IRQHandler();
}

void I2C2_ER_IRQHandler(void) {
    I2C_Platform_Error_IRQHandler();
}
