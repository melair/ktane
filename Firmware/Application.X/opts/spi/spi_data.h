#ifndef SPI_DATA_H
#define	SPI_DATA_H

#include "../../hal/pins.h"
#include "../../hal/spi.h"

#define SPI_RTC_BUFFER_SIZE 8
#define SPI_SDCARD_BUFFER_SIZE 10

#define SPI_SDCARD_CMD_SIZE 10
#define SPI_SDCARD_SCRATCH_SIZE 2
#define SPI_SDCARD_INIT_INBOUND_SIZE 4

#define SDCARD_CMD_INPROGRESS   0x00
#define SDCARD_CMD_ERROR        0x01
#define SDCARD_CMD_COMPLETE     0x02

typedef struct {
    uint8_t state;
    uint8_t response_type;
    
    uint8_t result;          
    
    uint8_t outbound_data[SPI_SDCARD_CMD_SIZE];
    uint8_t scratch_data[SPI_SDCARD_SCRATCH_SIZE];
    uint8_t *inbound_data;  
    
    spi_command_t spi_cmd;
    pin_t spi_cs;
} sd_transaction_t;

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
    
    struct {
        struct {
            unsigned ready         :1;
            unsigned high_capacity :1;
            
            uint8_t init_stage;
        } flags;
        
        struct {
            spi_device_t device;         
        } spi;                
                
        sd_transaction_t init_transaction;    
        uint8_t init_inbound_data[SPI_SDCARD_INIT_INBOUND_SIZE];
    } sdcard;    
} opt_spi_t;

#endif	/* SPI_DATA_H */

