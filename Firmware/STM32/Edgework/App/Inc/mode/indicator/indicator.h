#ifndef EDGEWORK_MODE_INDICATOR_H
#define EDGEWORK_MODE_INDICATOR_H

#include <stdbool.h>
#include <stdint.h>

#include "edgework/data.h"
#include "epaper/epaper.h"
#include "stm32g0xx_hal.h"

typedef struct Mode_Definition Mode_Definition;

#define INDICATOR_EPAPER_WIDTH  122U
#define INDICATOR_EPAPER_HEIGHT 250U
#define INDICATOR_EPAPER_FRAMEBUFFER_SIZE \
    ((((INDICATOR_EPAPER_WIDTH + 7U) / 8U) * INDICATOR_EPAPER_HEIGHT))

typedef struct {
    bool startup_refresh_started;
    bool display_refresh_started;
    edgework_state_t displayed_state;

    struct {
        SPI_HandleTypeDef handle;
        DMA_HandleTypeDef dma_handle;
    } spi;

    struct {
        Epaper display;
        uint8_t black[INDICATOR_EPAPER_FRAMEBUFFER_SIZE];
    } epaper;
} Indicator_Data;

extern Mode_Definition indicator_mode;

#endif // EDGEWORK_MODE_INDICATOR_H
