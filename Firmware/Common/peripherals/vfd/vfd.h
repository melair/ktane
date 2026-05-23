#ifndef VFD_H
#define VFD_H

#include "../../hal/pin.h"
#include "../../hal/spi.h"
#include "../../hal/spi_queue.h"
#include "../../utils/fsm.h"
#include <stdint.h>

#define VFD_SIZE 8

typedef struct {
    fsm_t fsm;

    pin_t en;
    pin_t reset;
    pin_t cs;

    spi_transaction_t spi_transaction;
    spi_queue_t spi_queue;
    uint8_t spi_buffer[1];

    uint8_t edit_buffer[VFD_SIZE];
    uint8_t work_buffer[VFD_SIZE];
    bool update_request;
} vfd_t;

void vfd_init(vfd_t *vfd, pin_t en, pin_t reset, pin_t cs);
void vfd_service(vfd_t *vfd);
void vfd_set(vfd_t *vfd, uint8_t i, uint8_t chr);
void vfd_update(vfd_t *vfd);

#endif