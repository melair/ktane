#ifndef SPI_DATA_H
#define	SPI_DATA_H

#include "../../hal/pins.h"
#include "../../hal/spi.h"

#define SPI_RTC_BUFFER_SIZE 8

typedef struct {
    struct {
        pin_t rtc_cs;
        pin_t sd_cs;
        pin_t nf24_cs;
        pin_t nf24_ce;
        pin_t i_r_q;
        pin_t sclk;
        pin_t mosi;
        pin_t miso;       
    } pins;

    struct {        
        unsigned rtc    :1; 
        unsigned sd     :1; 
        unsigned nf24   :1; 
    } present;
    
    struct {
        struct {
            unsigned in_irq :1;
            unsigned ready  :1;
        } flags;
        
        spi_device_t device;           
        spi_command_t one;
        spi_command_t two;
        
        uint8_t one_buffer[SPI_RTC_BUFFER_SIZE];
        uint8_t two_buffer[SPI_RTC_BUFFER_SIZE];
    } rtc;
    
} opts_spi_t;

#endif	/* SPI_DATA_H */

