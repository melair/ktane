#include "spi/spi_platform.h"

static SPI_HandleTypeDef spi_handle;
static DMA_HandleTypeDef spi_dma_handle;

static bool configure_clock(void) {
    __HAL_RCC_SPI4_CONFIG(RCC_SPI4CLKSOURCE_HSI);
    return true;
}

static void enable_peripheral_clock(void) {
    __HAL_RCC_SPI4_CLK_ENABLE();
}

const SPI_Hardware SPI_HARDWARE = {
    .spi_instance = SPI4,
    .spi_handle = &spi_handle,
    .sck = {SPI_SCK_Port, SPI_SCK_Pin},
    .sck_alternate = GPIO_AF5_SPI4,
    .miso = {SPI_MISO_Port, SPI_MISO_Pin},
    .miso_alternate = GPIO_AF5_SPI4,
    .mosi = {SPI_MOSI_Port, SPI_MOSI_Pin},
    .mosi_alternate = GPIO_AF5_SPI4,
    .kernel_clock_hz = HSI_VALUE,
    .dma_instance = GPDMA1_Channel1,
    .dma_handle = &spi_dma_handle,
    .dma_tx_request = GPDMA1_REQUEST_SPI4_TX,
    .dma_rx_request = GPDMA1_REQUEST_SPI4_RX,
    .dma_irq = GPDMA1_Channel1_IRQn,
    .spi_irq = SPI4_IRQn,
    .configure_clock = configure_clock,
    .enable_peripheral_clock = enable_peripheral_clock,
};

void GPDMA1_Channel1_IRQHandler(void) {
    SPI_Platform_DMA_IRQHandler();
}

void SPI4_IRQHandler(void) {
    SPI_Platform_SPI_IRQHandler();
}
