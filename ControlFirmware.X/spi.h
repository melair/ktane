#ifndef SPI_H
#define	SPI_H

#include <stdint.h>
#include "peripherals/ports.h"

#define SPI_BAUD_COUNT      6

#define SPI_BAUD_125_KHZ    0
#define SPI_BAUD_200_KHZ    1
#define SPI_BAUD_400_KHZ    2
#define SPI_BAUD_800_KHZ    3
#define SPI_BAUD_1600_KHZ   4
#define SPI_BAUD_3200_KHZ   5

typedef struct {
    uint8_t spi_miso_pps_port;
    uint8_t *spi_mosi_pps_addr;
    uint8_t *spi_clk_pps_addr;
    
    bool spi_cs_bounce_between;
    
    pin_t spi_cs;
    
    unsigned baud :3;        
} spi_device_t;

#define SPI_OPERATION_WRITE 0
#define SPI_OPERATION_READ  1

typedef struct spi_command_t spi_command_t;

struct spi_command_t {
    unsigned device         :4;
    unsigned operation      :3;
    unsigned in_progress    :1;
    
    uint8_t *buffer;
    uint16_t size;
    
    spi_command_t *(*callback)(spi_command_t *);
};

void spi_initialise(void);
void spi_service(void);
uint8_t spi_register(spi_device_t *d);
void spi_enqueue(spi_command_t *c);

#endif	/* SPI_H */

