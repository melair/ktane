#ifndef CARDSCAN_H
#define	CARDSCAN_H

#include "../../spi.h"

void cardscan_initialise(void);

#define CARDSCAN_SPI_BUFFER 32

typedef struct {
    uint8_t spi_buffer[CARDSCAN_SPI_BUFFER];
    spi_command_t pn532_cmd;
    uint8_t spi_device;
    
    uint8_t pn532_state;
    uint32_t pn532_wait_time;
} mode_cardscan_t;

#endif	/* CARDSCAN_H */

