#ifndef I2C_PLATFORM_H
#define I2C_PLATFORM_H

#include <stdbool.h>
#include <stdint.h>

#include "i2c/i2c.h"
#include "sys/gpio.h"

typedef struct {
    void *i2c_instance;
    void *i2c_handle;

    GPIO_PinDef scl;
    uint32_t scl_alternate;
    GPIO_PinDef sda;
    uint32_t sda_alternate;
    uint32_t timing;

    void *dma_instance;
    void *dma_handle;
    uint32_t dma_tx_request;
    uint32_t dma_rx_request;

    int32_t dma_irq;
    int32_t event_irq;
    int32_t error_irq;

    bool (*configure_clock)(void);
    void (*enable_peripheral_clock)(void);
} I2C_Hardware;

typedef struct {
    bool (*init)(const I2C_Hardware *hardware);
    bool (*start_write)(uint16_t address, uint8_t *data, uint16_t size, bool retain_bus);
    bool (*start_read)(uint16_t address, uint8_t *data, uint16_t size, bool follows_restart);
    bool (*transfer_cleanup)(void);
    void (*dma_irq)(void);
    void (*event_irq)(void);
    void (*error_irq)(void);
} I2C_Platform;

extern const I2C_Hardware I2C_HARDWARE;
extern const I2C_Platform I2C_PLATFORM;

void I2C_Platform_NotifyTransferComplete(void);
void I2C_Platform_NotifyTransferError(I2C_Status status);

void I2C_Platform_DMA_IRQHandler(void);
void I2C_Platform_Event_IRQHandler(void);
void I2C_Platform_Error_IRQHandler(void);

#endif //I2C_PLATFORM_H
