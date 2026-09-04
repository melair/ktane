#include "spi_platform.h"

static const SPI_Hardware *spi_hardware;

static SPI_HandleTypeDef *spi_handle(void) {
    return spi_hardware->spi_handle;
}

static DMA_HandleTypeDef *dma_handle(void) {
    return spi_hardware->dma_handle;
}

static bool baud_prescaler(SPI_Baud baud, uint32_t *value);

static bool hardware_valid(const SPI_Hardware *hardware) {
    return (hardware != NULL) &&
           (hardware->spi_instance != NULL) &&
           (hardware->spi_handle != NULL) &&
           (hardware->sck.port != NULL) &&
           (hardware->miso.port != NULL) &&
           (hardware->mosi.port != NULL) &&
           (hardware->kernel_clock_hz != 0U) &&
           (hardware->dma_instance != NULL) &&
           (hardware->dma_handle != NULL) &&
           (hardware->configure_clock != NULL) &&
           (hardware->enable_peripheral_clock != NULL);
}

static void gpio_configure(const GPIO_PinDef *pin, uint32_t alternate) {
    GPIO_InitTypeDef gpio_init = {0};

    gpio_init.Pin = pin->pin;
    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    gpio_init.Alternate = alternate;
    HAL_GPIO_Init(pin->port, &gpio_init);
}

static bool init(const SPI_Hardware *hardware) {
    if (!hardware_valid(hardware)) {
        return false;
    }

    spi_hardware = hardware;
    SPI_HandleTypeDef *hspi = spi_handle();
    uint32_t initial_prescaler = 0U;

    if (!hardware->configure_clock() || !baud_prescaler(SPI_BAUD_1MHZ, &initial_prescaler)) {
        return false;
    }

    hardware->enable_peripheral_clock();
    gpio_configure(&hardware->sck, hardware->sck_alternate);
    gpio_configure(&hardware->miso, hardware->miso_alternate);
    gpio_configure(&hardware->mosi, hardware->mosi_alternate);

    HAL_NVIC_SetPriority((IRQn_Type) hardware->dma_irq, 0U, 0U);
    HAL_NVIC_EnableIRQ((IRQn_Type) hardware->dma_irq);
    HAL_NVIC_SetPriority((IRQn_Type) hardware->spi_irq, 0U, 0U);
    HAL_NVIC_EnableIRQ((IRQn_Type) hardware->spi_irq);

    hspi->Instance = hardware->spi_instance;
    hspi->Init.Mode = SPI_MODE_MASTER;
    hspi->Init.Direction = SPI_DIRECTION_2LINES;
    hspi->Init.DataSize = SPI_DATASIZE_8BIT;
    hspi->Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi->Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi->Init.NSS = SPI_NSS_SOFT;
    hspi->Init.BaudRatePrescaler = initial_prescaler;
    hspi->Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi->Init.TIMode = SPI_TIMODE_DISABLE;
    hspi->Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi->Init.CRCPolynomial = 0x7U;
    hspi->Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
    hspi->Init.NSSPMode = SPI_NSS_PULSE_ENABLE;

    return HAL_SPI_Init(hspi) == HAL_OK;
}

static bool data_size(uint8_t bits, uint32_t *value) {
    switch (bits) {
        case 4U:
            *value = SPI_DATASIZE_4BIT;
            return true;
        case 5U:
            *value = SPI_DATASIZE_5BIT;
            return true;
        case 6U:
            *value = SPI_DATASIZE_6BIT;
            return true;
        case 7U:
            *value = SPI_DATASIZE_7BIT;
            return true;
        case 8U:
            *value = SPI_DATASIZE_8BIT;
            return true;
        case 9U:
            *value = SPI_DATASIZE_9BIT;
            return true;
        case 10U:
            *value = SPI_DATASIZE_10BIT;
            return true;
        case 11U:
            *value = SPI_DATASIZE_11BIT;
            return true;
        case 12U:
            *value = SPI_DATASIZE_12BIT;
            return true;
        case 13U:
            *value = SPI_DATASIZE_13BIT;
            return true;
        case 14U:
            *value = SPI_DATASIZE_14BIT;
            return true;
        case 15U:
            *value = SPI_DATASIZE_15BIT;
            return true;
        case 16U:
            *value = SPI_DATASIZE_16BIT;
            return true;
        default:
            return false;
    }
}

static bool baud_frequency(SPI_Baud baud, uint32_t *value) {
    switch (baud) {
        case SPI_BAUD_8MHZ:
            *value = 8000000U;
            return true;
        case SPI_BAUD_4MHZ:
            *value = 4000000U;
            return true;
        case SPI_BAUD_2MHZ:
            *value = 2000000U;
            return true;
        case SPI_BAUD_1MHZ:
            *value = 1000000U;
            return true;
        case SPI_BAUD_500KHZ:
            *value = 500000U;
            return true;
        case SPI_BAUD_250KHZ:
            *value = 250000U;
            return true;
        case SPI_BAUD_125KHZ:
            *value = 125000U;
            return true;
        default:
            return false;
    }
}

static bool baud_prescaler(SPI_Baud baud, uint32_t *value) {
    uint32_t frequency = 0U;

    if (!baud_frequency(baud, &frequency) || ((spi_hardware->kernel_clock_hz % frequency) != 0U)) {
        return false;
    }

    switch (spi_hardware->kernel_clock_hz / frequency) {
        case 2U:
            *value = SPI_BAUDRATEPRESCALER_2;
            return true;
        case 4U:
            *value = SPI_BAUDRATEPRESCALER_4;
            return true;
        case 8U:
            *value = SPI_BAUDRATEPRESCALER_8;
            return true;
        case 16U:
            *value = SPI_BAUDRATEPRESCALER_16;
            return true;
        case 32U:
            *value = SPI_BAUDRATEPRESCALER_32;
            return true;
        case 64U:
            *value = SPI_BAUDRATEPRESCALER_64;
            return true;
        case 128U:
            *value = SPI_BAUDRATEPRESCALER_128;
            return true;
        case 256U:
            *value = SPI_BAUDRATEPRESCALER_256;
            return true;
        default:
            return false;
    }
}

static bool configure(uint8_t bits, SPI_Baud baud, bool lsb_first, bool cke, bool ckp) {
    uint32_t configured_data_size = 0U;
    uint32_t configured_prescaler = 0U;

    if (!data_size(bits, &configured_data_size) || !baud_prescaler(baud, &configured_prescaler)) {
        return false;
    }

    SPI_HandleTypeDef *hspi = spi_handle();
    hspi->Init.Direction = SPI_DIRECTION_2LINES;
    hspi->Init.DataSize = configured_data_size;
    hspi->Init.CLKPolarity = ckp ? SPI_POLARITY_HIGH : SPI_POLARITY_LOW;
    hspi->Init.CLKPhase = cke ? SPI_PHASE_2EDGE : SPI_PHASE_1EDGE;
    hspi->Init.BaudRatePrescaler = configured_prescaler;
    hspi->Init.FirstBit = lsb_first ? SPI_FIRSTBIT_LSB : SPI_FIRSTBIT_MSB;

    return HAL_SPI_Init(hspi) == HAL_OK;
}

static bool dma_configure(bool receive, uint8_t bits) {
    DMA_HandleTypeDef *hdma = dma_handle();
    SPI_HandleTypeDef *hspi = spi_handle();

    if ((hdma->Instance != NULL) && (HAL_DMA_DeInit(hdma) != HAL_OK)) {
        return false;
    }

    hdma->Instance = spi_hardware->dma_instance;
    hdma->Init.Request = receive ? spi_hardware->dma_rx_request : spi_hardware->dma_tx_request;
    hdma->Init.Direction = receive ? DMA_PERIPH_TO_MEMORY : DMA_MEMORY_TO_PERIPH;
    hdma->Init.PeriphInc = DMA_PINC_DISABLE;
    hdma->Init.MemInc = DMA_MINC_ENABLE;
    hdma->Init.PeriphDataAlignment = bits > 8U ? DMA_PDATAALIGN_HALFWORD : DMA_PDATAALIGN_BYTE;
    hdma->Init.MemDataAlignment = bits > 8U ? DMA_MDATAALIGN_HALFWORD : DMA_MDATAALIGN_BYTE;
    hdma->Init.Mode = DMA_NORMAL;
    hdma->Init.Priority = DMA_PRIORITY_HIGH;

    if (HAL_DMA_Init(hdma) != HAL_OK) {
        return false;
    }

    if (receive) {
        __HAL_LINKDMA(hspi, hdmarx, *hdma);
    } else {
        __HAL_LINKDMA(hspi, hdmatx, *hdma);
    }

    return true;
}

static bool set_direction(uint32_t direction) {
    SPI_HandleTypeDef *hspi = spi_handle();

    if (hspi->Init.Direction == direction) {
        return true;
    }

    hspi->Init.Direction = direction;
    return HAL_SPI_Init(hspi) == HAL_OK;
}

static bool start_write(void *data, uint16_t size, uint8_t bits) {
    return set_direction(SPI_DIRECTION_2LINES) &&
           dma_configure(false, bits) &&
           (HAL_SPI_Transmit_DMA(spi_handle(), data, size) == HAL_OK);
}

static bool start_read(void *data, uint16_t size, uint8_t bits) {
    return set_direction(SPI_DIRECTION_2LINES_RXONLY) &&
           dma_configure(true, bits) &&
           (HAL_SPI_Receive_DMA(spi_handle(), data, size) == HAL_OK);
}

static bool transfer_cleanup(void) {
    DMA_HandleTypeDef *hdma = dma_handle();

    return (hdma->Instance == NULL) || (HAL_DMA_DeInit(hdma) == HAL_OK);
}

static void chip_select(void *port, uint32_t pin, bool asserted) {
    HAL_GPIO_WritePin(port, pin, asserted ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

static void dma_irq(void) {
    HAL_DMA_IRQHandler(dma_handle());
}

static void spi_irq(void) {
    HAL_SPI_IRQHandler(spi_handle());
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
    if ((spi_hardware != NULL) && (hspi == spi_handle())) {
        SPI_Platform_NotifyTransferComplete();
    }
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi) {
    if ((spi_hardware != NULL) && (hspi == spi_handle())) {
        SPI_Platform_NotifyTransferComplete();
    }
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi) {
    if ((spi_hardware != NULL) && (hspi == spi_handle())) {
        SPI_Platform_NotifyTransferError();
    }
}

const SPI_Platform SPI_PLATFORM = {
    .init = init,
    .configure = configure,
    .start_write = start_write,
    .start_read = start_read,
    .transfer_cleanup = transfer_cleanup,
    .chip_select = chip_select,
    .dma_irq = dma_irq,
    .spi_irq = spi_irq,
};
