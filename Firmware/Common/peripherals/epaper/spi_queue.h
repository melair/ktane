#ifndef SPI_QUEUE_H
#define SPI_QUEUE_H

#define SPI_QUEUE_BUFFER_SIZE 4

#include <stdint.h>
#include <stdbool.h>
#include "../../hal/pin.h"
#include "../../hal/spi.h"

typedef struct {
  unsigned data : 1;
  unsigned size : 3;
  uint8_t buffer[SPI_QUEUE_BUFFER_SIZE];
} spi_queue_t;

bool spi_queue_process(const spi_queue_t (*queue)[], uint8_t queue_size, pin_t data, spi_transaction_t *spi, uint8_t *phase);
#endif