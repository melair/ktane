#ifndef PN532_SPI_H
#define	PN532_SPI_H

#include <stdint.h>
#include <stdbool.h>

void pn532_spi_service(void);
void pn532_spi_send(uint8_t *buff, uint8_t write_size, uint8_t read_size, void (*callback)(bool));


#endif	/* PN532_SPI_H */

