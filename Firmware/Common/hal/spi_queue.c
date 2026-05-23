#include "spi_queue.h"
#include <xc.h>
#include "pin.h"
#include "spi.h"
#include "../utils/mem.h"

void spi_queue_init(spi_queue_t *sq, const spi_queue_element_t (*queue)[], uint8_t queue_size, pin_t data) {
    memset(sq, 0, sizeof(spi_transaction_t));

    sq->queue = queue;
    sq->queue_size = queue_size;
    sq->data = data;
}

bool spi_queue_process(spi_queue_t *sq, spi_transaction_t *spi) {
    if (sq->phase >= sq->queue_size) {
        if (sq->data != PORTPIN_NONE) {
            pin_write(sq->data, false);
        }
        return false;
    }

    const spi_queue_element_t *element = &(*sq->queue)[sq->phase];

    if (sq->data != PORTPIN_NONE) {
        pin_write(sq->data, element->data == 1);
    }

    memcpy(element->buffer, spi->buffer, element->size);

    spi->write_size = element->size;
    spi->cs_bounce = element->cs_bounce;
    sq->phase++;

    return true;
}
