#ifndef EDGEWORK_SPI_CONFIG_H
#define EDGEWORK_SPI_CONFIG_H

#include "spi/spi_platform.h"

/* Bind SPI1 to the active mode's handles before calling SPI_Init(). */
void SPI_Config(SPI_HandleTypeDef *handle, DMA_HandleTypeDef *dma_handle);

#endif // EDGEWORK_SPI_CONFIG_H
