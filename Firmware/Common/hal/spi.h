#ifndef SPI_H
#define	SPI_H

#include <stdint.h>
#include <stdbool.h>
#include "pin.h"
#include "../utils/fsm.h"

#define SPI1 0b00001000
#define SPI2 0b00010000

#define SPI_BITS 0b00011000
#define SPI_NUM(NUM)      ((uint8_t)((NUM & SPI_BITS) >> 3))

typedef struct spi_transaction_t spi_transaction_t;
typedef struct spi_t spi_t;

typedef struct {
    uint8_t num;
    volatile uint8_t *clk;
    volatile uint8_t *con0;
    volatile uint8_t *con1;
    volatile uint8_t *con2;
    volatile uint8_t *twidth;
    volatile uint8_t *inte;
    volatile uint8_t *baud;
    volatile uint8_t *intf;
    volatile uint8_t *status;
    volatile uint8_t *tcntl;
    volatile uint8_t *tcnth;
    volatile uint8_t *txb;
    volatile uint8_t *rxb;

    uint8_t spitx_vector;
    uint8_t spirx_vector;
} spi_peripheral_t;

struct spi_t {
    fsm_t fsm;

    const spi_peripheral_t *spi_peripheral;
    uint8_t dma_peripheral;

    spi_transaction_t *queue_head;
    spi_transaction_t *queue_tail;

    spi_transaction_t *current;
    pin_t current_cs_pin;
};

#define SPI_OPERATION_WRITE           0
#define SPI_OPERATION_WRITE_THEN_READ 1
#define SPI_OPERATION_READ            2

struct spi_transaction_t {
    unsigned operation :3;

    uint8_t *buffer;
    uint16_t write_size;
    uint16_t read_size;

    unsigned bits :3;
    uint24_t baud;
    bool lsb_first;
    bool cke;

    pin_t cs_pin;
    bool cs_bounce;
    uint8_t cs_wait_ms;
    
    spi_transaction_t *(*callback)(spi_transaction_t *);
    void *callback_data;

    spi_transaction_t *queue_next;
};

void spi_init(spi_t *spi, pin_t copi, pin_t clk, pin_t cipo, uint8_t config);
void spi_service(spi_t *spi);
void spi_interrupt(spi_t *spi);
void spi_queue(spi_t *spi, spi_transaction_t *transaction);

#endif