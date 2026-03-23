#ifndef DMA_H
#define	DMA_H

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>

#define DMA1 0x00
#define DMA2 0x01
#define DMA3 0x02
#define DMA4 0x03

#ifdef DMA5PR
#define DMA5 0x04
#endif

#ifdef DMA6PR
#define DMA6 0x05
#endif

#ifdef DMA7PR
#define DMA7 0x06
#endif

#ifdef DMA8PR
#define DMA8 0x07
#endif

#define DMA_BITS 0b00000111
#define DMA_NUM(NUM)      ((uint8_t)((NUM & DMA_BITS)))

void dma_inte_dcnt(uint8_t dma, bool en);
bool dma_intf_dcnt(uint8_t dma);

#define DMA_SELECT_BEGIN(channel) \
    uint8_t _dmaselect_saved = DMASELECT; \
    DMASELECT = (channel)

#define DMA_SELECT_END() \
    DMASELECT = _dmaselect_saved

#endif