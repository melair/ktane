#ifndef ARGB_INTERNAL_H
#define ARGB_INTERNAL_H

#include <xc.h>

#define PWM1 1
#define PWM2 2
#define PWM3 3
#define PWM4 4

// PWM
#ifndef ARGB_PWM_PERIPHERAL
#define ARGB_PWM_PERIPHERAL PWM1
#warning No ARGB PWM defined, assuming PWM1.
#endif

#if ARGB_PWM_PERIPHERAL == PWM1
#define ARGB_PWM_CLK PWM1CLKbits.CLK
#define ARGB_PWM_PR PWM1PR
#define ARGB_PWM_S1P1 PWM1S1P1
#define ARGB_PWM_EN PWM1CONbits.EN
#elif ARGB_PWM_PERIPHERAL == PWM2
#define ARGB_PWM_CLK PWM2CLKbits.CLK
#define ARGB_PWM_PR PWM2PR
#define ARGB_PWM_S1P1 PWM2S1P1
#define ARGB_PWM_EN PWM2CONbits.EN
#elif ARGB_PWM_PERIPHERAL == PWM3
#define ARGB_PWM_CLK PWM3CLKbits.CLK
#define ARGB_PWM_PR PWM3PR
#define ARGB_PWM_S1P1 PWM3S1P1
#define ARGB_PWM_EN PWM3CONbits.EN
#elif ARGB_PWM_PERIPHERAL == PWM4
#define ARGB_PWM_CLK PWM4CLKbits.CLK
#define ARGB_PWM_PR PWM4PR
#define ARGB_PWM_S1P1 PWM4S1P1
#define ARGB_PWM_EN PWM4CONbits.EN
#endif 

#define CLC1 0
#define CLC2 1
#define CLC3 2
#define CLC4 3
#define CLC5 4
#define CLC6 5
#define CLC7 6
#define CLC8 7

#if defined(_PIC18F15Q40_H_)
#define CLC_PWM_BASE 21
#elif defined(_PIC18F57Q84_H_) || defined(_PIC18F25Q43_H_)
#define CLC_PWM_BASE 66
#else
#error Unsupported PIC, requires CLC PWM base number.
#endif

// CLC - PWM->SPI
#ifndef ARGB_CLC_PWM_SPI_PERIPHERAL
#define ARGB_CLC_PWM_SPI_PERIPHERAL CLC0
#warning No ARGB CLC (PWM->SPI) defined, assuming CLC1.
#endif

// SPI

#define SPI1 0
#define SPI2 1

#ifndef ARGB_SPI_PERIPHERAL
#define ARGB_SPI_PERIPHERAL SPI1
#warning No ARGB SPI peripheral defined, assuming SPI1 - MAY CLASH WITH SPI HAL!
#endif

#if ARGB_SPI_PERIPHERAL == SPI1
#define ARGB_SPI_CON_TXR SPI1CON2bits.TXR
#define ARGB_SPI_CLK SPI1CLK
#define ARGB_SPI_BAUD SPI1BAUD
#define ARGB_SPI_CON_MST SPI1CON0bits.MST
#define ARGB_SPI_CON_BMODE SPI1CON0bits.BMODE
#define ARGB_SPI_CON_EN SPI1CON0bits.EN
#define ARGB_SPI_TXB SPI1TXB
#define SPITXVECTOR 0x19
#else
#define ARGB_SPI_CON_TXR SPI2CON2bits.TXR
#define ARGB_SPI_CLK SPI2CLK
#define ARGB_SPI_BAUD SPI2BAUD
#define ARGB_SPI_CON_MST SPI2CON0bits.MST
#define ARGB_SPI_CON_BMODE SPI2CON0bits.BMODE
#define ARGB_SPI_CON_EN SPI2CON0bits.EN
#define ARGB_SPI_TXB SPI2TXB
#define SPITXVECTOR 0x29
#endif 

#if defined(_PIC18F15Q40_H_)
#define SPI_CLK_CLC_BASE 0b1001
#elif defined(_PIC18F57Q84_H_) || defined(_PIC18F25Q43_H_)
#define SPI_CLK_CLC_BASE 0b01111
#else
#error Unsupported PIC, requires CLC PWM base number.
#endif

// CLC2
#ifndef ARGB_CLC_SPI_OUT_PERIPHERAL
#define ARGB_CLC_SPI_OUT_PERIPHERAL CLC1
#warning No ARGB CLC (SPI->OUT) defined, assuming CLC2 - CHECK PIN PPS MAPPINGS!
#endif

#if defined(_PIC18F15Q40_H_)
#define CLC_SPI_SDO_BASE 41
#define CLC_SPI_SCK_BASE 42
#elif defined(_PIC18F57Q84_H_) || defined(_PIC18F25Q43_H_)
#define CLC_SPI_SDO_BASE 64
#define CLC_SPI_SCK_BASE 65
#else
#error Unsupported PIC, requires CLC SPI SDO/SCK base number.
#endif

#ifndef ARGB_DMA
#define ARGB_DMA DMA3
#warning No ARGB DMA defined, assuming DMA3.
#endif

#endif