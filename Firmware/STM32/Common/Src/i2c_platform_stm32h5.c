#include "i2c_platform.h"

static const I2C_Hardware *i2c_hardware;

static I2C_HandleTypeDef *i2c_handle(void) {
    return i2c_hardware->i2c_handle;
}

static DMA_HandleTypeDef *dma_handle(void) {
    return i2c_hardware->dma_handle;
}

static bool hardware_valid(const I2C_Hardware *hardware) {
    return (hardware != NULL) &&
           (hardware->i2c_instance != NULL) &&
           (hardware->i2c_handle != NULL) &&
           (hardware->scl.port != NULL) &&
           (hardware->sda.port != NULL) &&
           (hardware->dma_instance != NULL) &&
           (hardware->dma_handle != NULL) &&
           (hardware->configure_clock != NULL) &&
           (hardware->enable_peripheral_clock != NULL);
}

static void gpio_configure(const GPIO_PinDef *pin, uint32_t alternate) {
    GPIO_InitTypeDef gpio_init = {0};

    gpio_init.Pin = pin->pin;
    gpio_init.Mode = GPIO_MODE_AF_OD;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    gpio_init.Alternate = alternate;
    HAL_GPIO_Init(pin->port, &gpio_init);
}

static bool init(const I2C_Hardware *hardware) {
    if (!hardware_valid(hardware)) {
        return false;
    }

    i2c_hardware = hardware;
    I2C_HandleTypeDef *hi2c = i2c_handle();

    if (!hardware->configure_clock()) {
        return false;
    }

    hardware->enable_peripheral_clock();
    gpio_configure(&hardware->scl, hardware->scl_alternate);
    gpio_configure(&hardware->sda, hardware->sda_alternate);

    HAL_NVIC_SetPriority((IRQn_Type) hardware->dma_irq, 0, 0);
    HAL_NVIC_EnableIRQ((IRQn_Type) hardware->dma_irq);
    HAL_NVIC_SetPriority((IRQn_Type) hardware->event_irq, 0, 0);
    HAL_NVIC_EnableIRQ((IRQn_Type) hardware->event_irq);
    HAL_NVIC_SetPriority((IRQn_Type) hardware->error_irq, 0, 0);
    HAL_NVIC_EnableIRQ((IRQn_Type) hardware->error_irq);

    hi2c->Instance = hardware->i2c_instance;
    hi2c->Init.Timing = hardware->timing;
    hi2c->Init.OwnAddress1 = 0;
    hi2c->Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c->Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c->Init.OwnAddress2 = 0;
    hi2c->Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    hi2c->Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c->Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    return (HAL_I2C_Init(hi2c) == HAL_OK) &&
           (HAL_I2CEx_ConfigAnalogFilter(hi2c, I2C_ANALOGFILTER_ENABLE) == HAL_OK) &&
           (HAL_I2CEx_ConfigDigitalFilter(hi2c, 0) == HAL_OK);
}

static bool dma_configure(bool receive) {
    DMA_HandleTypeDef *hdma = dma_handle();
    I2C_HandleTypeDef *hi2c = i2c_handle();

    if ((hdma->Instance != NULL) && (HAL_DMA_DeInit(hdma) != HAL_OK)) {
        return false;
    }

    hdma->Instance = i2c_hardware->dma_instance;
    hdma->Init.Request = receive ? i2c_hardware->dma_rx_request : i2c_hardware->dma_tx_request;
    hdma->Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    hdma->Init.Direction = receive ? DMA_PERIPH_TO_MEMORY : DMA_MEMORY_TO_PERIPH;
    hdma->Init.SrcInc = receive ? DMA_SINC_FIXED : DMA_SINC_INCREMENTED;
    hdma->Init.DestInc = receive ? DMA_DINC_INCREMENTED : DMA_DINC_FIXED;
    hdma->Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    hdma->Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    hdma->Init.Priority = DMA_LOW_PRIORITY_HIGH_WEIGHT;
    hdma->Init.SrcBurstLength = 1;
    hdma->Init.DestBurstLength = 1;
    hdma->Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0;
    hdma->Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    hdma->Init.Mode = DMA_NORMAL;

    if ((HAL_DMA_Init(hdma) != HAL_OK) ||
        (HAL_DMA_ConfigChannelAttributes(hdma, DMA_CHANNEL_NPRIV) != HAL_OK)) {
        return false;
    }

    if (receive) {
        __HAL_LINKDMA(hi2c, hdmarx, *hdma);
    } else {
        __HAL_LINKDMA(hi2c, hdmatx, *hdma);
    }

    return true;
}

static bool start_write(uint16_t address, uint8_t *data, uint16_t size, bool retain_bus) {
    if (!dma_configure(false)) {
        return false;
    }

    if (retain_bus) {
        return HAL_I2C_Master_Seq_Transmit_DMA(i2c_handle(), address, data, size, I2C_FIRST_FRAME) == HAL_OK;
    }

    return HAL_I2C_Master_Transmit_DMA(i2c_handle(), address, data, size) == HAL_OK;
}

static bool start_read(uint16_t address, uint8_t *data, uint16_t size, bool follows_restart) {
    if (!dma_configure(true)) {
        return false;
    }

    if (follows_restart) {
        return HAL_I2C_Master_Seq_Receive_DMA(i2c_handle(), address, data, size, I2C_LAST_FRAME) == HAL_OK;
    }

    return HAL_I2C_Master_Receive_DMA(i2c_handle(), address, data, size) == HAL_OK;
}

static bool transfer_cleanup(void) {
    DMA_HandleTypeDef *hdma = dma_handle();

    return (hdma->Instance == NULL) || (HAL_DMA_DeInit(hdma) == HAL_OK);
}

static void dma_irq(void) {
    HAL_DMA_IRQHandler(dma_handle());
}

static void event_irq(void) {
    HAL_I2C_EV_IRQHandler(i2c_handle());
}

static void error_irq(void) {
    HAL_I2C_ER_IRQHandler(i2c_handle());
}

static I2C_Status error_status(void) {
    const uint32_t error = HAL_I2C_GetError(i2c_handle());

    if ((error & HAL_I2C_ERROR_AF) != 0U) {
        return I2C_STATUS_ERROR_NACK;
    }

    if ((error & HAL_I2C_ERROR_TIMEOUT) != 0U) {
        return I2C_STATUS_ERROR_BTO;
    }

    return I2C_STATUS_ERROR_UNKNOWN;
}

void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c) {
    if ((i2c_hardware != NULL) && (hi2c == i2c_handle())) {
        I2C_Platform_NotifyTransferComplete();
    }
}

void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c) {
    if ((i2c_hardware != NULL) && (hi2c == i2c_handle())) {
        I2C_Platform_NotifyTransferComplete();
    }
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c) {
    if ((i2c_hardware != NULL) && (hi2c == i2c_handle())) {
        I2C_Platform_NotifyTransferError(error_status());
    }
}

const I2C_Platform I2C_PLATFORM = {
    .init = init,
    .start_write = start_write,
    .start_read = start_read,
    .transfer_cleanup = transfer_cleanup,
    .dma_irq = dma_irq,
    .event_irq = event_irq,
    .error_irq = error_irq,
};
