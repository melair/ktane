#ifndef I2C_INTERNAL_H
#define I2C_INTERNAL_H

#include <xc.h>

#ifndef I2C_DMA
#define I2C_DMA DMA2
#warning No I2C DMA defined, assuming DMA2.
#endif

#if defined(_PIC18F15Q40_H_)
#define I2CDATPPS 0x22
#define I2CCLKPPS 0x21
#elif defined(_PIC18F57Q84_H_)
#define I2CDATPPS 0x38
#define I2CCLKPPS 0x37
#elif defined(_PIC18F25Q43_H_)
#define I2CDATPPS 0x38
#define I2CCLKPPS 0x37
#else
#error Unsupported PIC, requires PPS values.
#endif

#if !defined(I2C1CNT) && !defined(I2C1CNTL)
#error Unsupported PIC, I2C transaction counter unknown.
#endif

#endif