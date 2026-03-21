#include "dma.h"
#include <stdbool.h>
#include <stdint.h>
#include <xc.h>

void dma_inte_dcnt(uint8_t dma, bool en) {
  switch (dma) {
  case DMA1:
    DMA1DCNTIE = (en ? 1 : 0);
    break;
  case DMA2:
    DMA2DCNTIE = (en ? 1 : 0);
    break;
  case DMA3:
    DMA3DCNTIE = (en ? 1 : 0);
    break;
  case DMA4:
    DMA4DCNTIE = (en ? 1 : 0);
    break;
#ifdef DMA5
  case DMA5:
    DMA5DCNTIE = (en ? 1 : 0);
    break;
#endif
#ifdef DMA6
  case DMA6:
    DMA6DCNTIE = (en ? 1 : 0);
    break;
#endif
#ifdef DMA7
  case DMA7:
    DMA7DCNTIE = (en ? 1 : 0);
    break;
#endif
#ifdef DMA8
  case DMA8:
    DMA8DCNTIE = (en ? 1 : 0);
    break;
#endif
  }
}

bool dma_intf_dcnt(uint8_t dma) {
  bool val = false;

  switch (dma) {
  case DMA1:
    val = DMA1DCNTIF;
    DMA1DCNTIF = 0;
    break;
  case DMA2:
    val = DMA2DCNTIF;
    DMA2DCNTIF = 0;
    break;
  case DMA3:
    val = DMA3DCNTIF;
    DMA3DCNTIF = 0;
    break;
  case DMA4:
    val = DMA4DCNTIF;
    DMA4DCNTIF = 0;
    break;
#ifdef DMA5
  case DMA5:
    val = DMA5DCNTIF;
    DMA5DCNTIF = 0;
    break;
#endif
#ifdef DMA6
  case DMA6:
    val = DMA6DCNTIF;
    DMA6DCNTIF = 0;
    break;
#endif
#ifdef DMA7
  case DMA7:
    val = DMA7DCNTIF;
    DMA7DCNTIF = 0;
    break;
#endif
#ifdef DMA8
  case DMA8:
    val = DMA8DCNTIF;
    DMA8DCNTIF = 0;
    break;
#endif
  }

  return val;
}