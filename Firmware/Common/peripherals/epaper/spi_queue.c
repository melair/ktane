#include "spi_queue.h"
#include <xc.h>
#include "../../hal/spi.h"

bool spi_queue_process(const spi_queue_t (*queue)[], uint8_t queue_size, pin_t data, spi_transaction_t *spi, uint16_t *phase) {
    if (*phase >= queue_size) {
        pin_write(data, false);
        return false;
    }

    pin_write(data, (*queue)[*phase].data == 1);

    for (uint8_t j = 0; j < (*queue)[*phase].size; j++) {
        spi->buffer[j] = (*queue)[*phase].buffer[j];
    }

    spi->write_size = (*queue)[*phase].size;

    (*phase)++;

    return true;
}