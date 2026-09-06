#ifndef ARGB_PLATFORM_H
#define ARGB_PLATFORM_H

#include <stdbool.h>
#include <stdint.h>

#include "sys/gpio.h"

typedef struct {
    void *timer_instance;
    uint32_t timer_channel;
    uint32_t timer_clock_hz;

    GPIO_PinDef data;
    uint32_t data_alternate;

    void *dma_instance;
    uint32_t dma_request;
    int32_t dma_irq;

    void (*enable_timer_clock)(void);
} ARGB_Hardware;

typedef struct {
    bool (*init)(const ARGB_Hardware *hardware, uint16_t period_ticks);
    bool (*start)(const uint8_t *data, uint16_t length);
    void (*stop)(void);
    void (*dma_irq)(void);
} ARGB_Platform;

extern const ARGB_Hardware ARGB_HARDWARE;
extern const ARGB_Platform ARGB_PLATFORM;

void ARGB_Platform_NotifyHalfComplete(void);
void ARGB_Platform_NotifyTransferComplete(void);

void ARGB_Platform_DMA_IRQHandler(void);

#endif // ARGB_PLATFORM_H
