#ifndef SPI_H
#define	SPI_H

#include <stdint.h>
#include <stdbool.h>
#include "pin.h"
#include "../utils/fsm.h"

typedef struct spi_transaction_t spi_transaction_t;
typedef struct spi_t spi_t;

struct spi_t {
    fsm_t fsm;

    uint8_t dma_peripheral;

    spi_transaction_t *queue_head;
    spi_transaction_t *queue_tail;

    spi_transaction_t *current;

    pin_t current_cs_pin;
    uint32_t wait_until;
    unsigned RW_DONE :1;
};

#define SPI_OPERATION_WRITE           0
#define SPI_OPERATION_WRITE_THEN_READ 1
#define SPI_OPERATION_READ            2

#define SPI_BAUD_125K 255
#define SPI_BAUD_250K 129
#define SPI_BAUD_500K 65
#define SPI_BAUD_1000K 33
#define SPI_BAUD_2000K 17

struct spi_transaction_t {
    unsigned operation :3;

    uint8_t *buffer;
    uint16_t write_size;
    uint16_t write_repeats;
    uint16_t read_size;

    unsigned bits :3;
    uint8_t baud;
    bool lsb_first;
    bool cke;

    pin_t cs_pin;
    bool cs_bounce;
    uint8_t cs_wait_ms;
    
    spi_transaction_t *(*callback)(spi_transaction_t *);
    void *callback_data;

    spi_transaction_t *queue_next;
};

void spi_init(pin_t copi, pin_t clk, pin_t cipo, uint8_t config);
void spi_service(void);
void spi_interrupt(void);
void spi_queue(spi_transaction_t *transaction);

#endif