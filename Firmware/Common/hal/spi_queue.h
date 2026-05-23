#ifndef SPI_QUEUE_H
#define SPI_QUEUE_H

#include <stdint.h>
#include <stdbool.h>
#include "pin.h"
#include "spi.h"

typedef struct {
  unsigned cs_bounce : 1;
  unsigned data : 1;

  uint8_t size;
  const uint8_t *buffer;
} spi_queue_element_t;

typedef struct {
    pin_t data;

    const spi_queue_element_t (*queue)[];
    uint8_t queue_size;

    uint8_t phase;
} spi_queue_t;

void spi_queue_init(spi_queue_t *sq, const spi_queue_element_t (*queue)[], uint8_t queue_size, pin_t data);
bool spi_queue_process(spi_queue_t *sq, spi_transaction_t *spi);

#define BUFFER(...) \
    .size = sizeof((const uint8_t[]){__VA_ARGS__}), \
    .buffer = (const uint8_t[]){__VA_ARGS__}

#endif
