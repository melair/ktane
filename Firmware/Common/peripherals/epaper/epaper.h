#ifndef EPAPER_H
#define EPAPER_H

#include <stdbool.h>

#include "../../hal/pin.h"
#include "../../utils/fsm.h"
#include "../../hal/spi.h"

#define EPAPER_TYPE_SSD1680 0

typedef struct epaper_command_t epaper_command_t;
typedef struct epaper_t epaper_t;

#define OPERATION_FILL_WHITE    0
#define OPERATION_FILL_BLACK    1
#define OPERATION_FILL_RED      2

struct epaper_command_t {
    uint8_t operation;
    epaper_command_t *next;
};

struct epaper_t {
    pin_t cs;
    pin_t pwr;
    pin_t dc;
    pin_t reset;
    pin_t busy;

    uint8_t type;
    uint16_t width;
    uint16_t height;

    fsm_t fsm;

    epaper_command_t *queue_head;
    epaper_command_t *queue_tail;

    epaper_command_t *commited;
    bool partial;

    spi_transaction_t spi_transaction;
    uint8_t cmd_buffer[4];
    uint8_t phase;
};

void epaper_init(epaper_t *epaper);
void epaper_service(epaper_t *epaper);
void epaper_queue(epaper_t *epaper, epaper_command_t *cmd);
void epaper_refresh(epaper_t *epape, bool partial);

#endif