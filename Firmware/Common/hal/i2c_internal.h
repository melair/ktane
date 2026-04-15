#ifndef I2C_INTERNAL_H
#define I2C_INTERNAL_H

#include <xc.h>

#ifndef I2C_DMA
#define I2C_DMA DMA2
#warning No I2C DMA defined, assuming DMA2.
#endif

#if !defined(I2C1CNT) && !defined(I2C1CNTL)
#error Unsupported PIC, I2C transaction counter unknown.
#endif

#endif