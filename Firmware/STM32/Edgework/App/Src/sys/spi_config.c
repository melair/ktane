#include "sys/spi_config.h"

static bool configure_clock(void) {
    /* SPI1 uses the 64 MHz APB clock. */
    return true;
}

static void enable_peripheral_clock(void) {
    __HAL_RCC_SPI1_CLK_ENABLE();
}

SPI_Hardware SPI_HARDWARE = {
    .spi_instance = SPI1,
    .sck = {SPI_SCK_Port, SPI_SCK_Pin},
    .sck_alternate = GPIO_AF0_SPI1,
    .mosi = {SPI_MOSI_Port, SPI_MOSI_Pin},
    .mosi_alternate = GPIO_AF0_SPI1,
    .kernel_clock_hz = 64000000U,
    .dma_instance = DMA1_Channel4,
    .dma_tx_request = DMA_REQUEST_SPI1_TX,
    .dma_irq = DMA1_Ch4_7_DMAMUX1_OVR_IRQn,
    .spi_irq = SPI1_IRQn,
    .configure_clock = configure_clock,
    .enable_peripheral_clock = enable_peripheral_clock,
};

void SPI_Config(SPI_HandleTypeDef *handle, DMA_HandleTypeDef *dma_handle) {
    SPI_HARDWARE.spi_handle = handle;
    SPI_HARDWARE.dma_handle = dma_handle;
}

void DMA1_Ch4_7_DMAMUX1_OVR_IRQHandler(void) {
    SPI_Platform_DMA_IRQHandler();
}

void SPI1_IRQHandler(void) {
    SPI_Platform_SPI_IRQHandler();
}
