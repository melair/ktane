#ifndef SPI_PLATFORM_H
#define SPI_PLATFORM_H

#include <stdbool.h>
#include <stdint.h>

#include "spi.h"
#include "sys/gpio.h"

typedef struct {
    void *spi_instance;
    void *spi_handle;

    GPIO_PinDef sck;
    uint32_t sck_alternate;
    GPIO_PinDef miso;
    uint32_t miso_alternate;
    GPIO_PinDef mosi;
    uint32_t mosi_alternate;
    uint32_t kernel_clock_hz;

    void *dma_instance;
    void *dma_handle;
    uint32_t dma_tx_request;
    uint32_t dma_rx_request;

    int32_t dma_irq;
    int32_t spi_irq;

    bool (*configure_clock)(void);
    void (*enable_peripheral_clock)(void);
} SPI_Hardware;

typedef struct {
    bool (*init)(const SPI_Hardware *hardware);
    bool (*configure)(uint8_t bits, SPI_Baud baud, bool lsb_first, bool cke, bool ckp);
    bool (*start_write)(void *data, uint16_t size, uint8_t bits);
    bool (*start_read)(void *data, uint16_t size, uint8_t bits);
    bool (*transfer_cleanup)(void);
    void (*chip_select)(void *port, uint32_t pin, bool asserted);
    void (*dma_irq)(void);
    void (*spi_irq)(void);
} SPI_Platform;

extern const SPI_Hardware SPI_HARDWARE;
extern const SPI_Platform SPI_PLATFORM;

void SPI_Platform_NotifyTransferComplete(void);
void SPI_Platform_NotifyTransferError(void);

void SPI_Platform_DMA_IRQHandler(void);
void SPI_Platform_SPI_IRQHandler(void);

#endif //SPI_PLATFORM_H
