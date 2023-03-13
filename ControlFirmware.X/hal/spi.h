#ifndef SPI_H
#define	SPI_H

#include <stdint.h>
#include <stdbool.h>
#include "pins.h"

#define SPI_BAUD_COUNT      6

#define SPI_BAUD_125_KHZ    0
#define SPI_BAUD_200_KHZ    1
#define SPI_BAUD_400_KHZ    2
#define SPI_BAUD_800_KHZ    3
#define SPI_BAUD_1600_KHZ   4
#define SPI_BAUD_3200_KHZ   5

typedef struct {
    pin_t miso_pin;
    pin_t mosi_pin;
    pin_t clk_pin;
    pin_t cs_pin;

    bool cs_bounce;
            
    uint8_t baud;     
    bool lsb_first;
    bool cke;
} spi_device_t;

#define SPI_OPERATION_WRITE           0
#define SPI_OPERATION_WRITE_THEN_READ 1
#define SPI_OPERATION_READ            2

typedef struct spi_command_t spi_command_t;

struct spi_command_t {
    unsigned operation      :3;
    unsigned in_progress    :1;
    
    uint8_t *buffer;
    uint16_t write_size;
    uint16_t read_size;
    
    uint16_t cs_delay;
    
    spi_command_t *(*callback)(spi_command_t *);
    void *callback_ptr;
    
    spi_command_t *next_cmd;
    spi_device_t *device;
};

void spi_initialise(void);
void spi_service(void);
void spi_enqueue(spi_command_t *c);

#endif	/* SPI_H */

