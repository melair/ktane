#ifndef SPI_H
#define SPI_H
#include <stdbool.h>
#include "stm32h562xx.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SPI_BAUD_8MHZ = 0,
    SPI_BAUD_4MHZ,
    SPI_BAUD_2MHZ,
    SPI_BAUD_1MHZ,
    SPI_BAUD_500KHZ,
    SPI_BAUD_250KHZ,
    SPI_BAUD_125KHZ
} SPI_Baud;

typedef enum {
    SPI_OPERATION_WRITE = 0,
    SPI_OPERATION_WRITE_THEN_READ,
    SPI_OPERATION_READ,
} SPI_Operation;

typedef enum {
    SPI_STATE_IDLE = 0,
    SPI_STATE_TX,
    SPI_STATE_RX,
    SPI_STATE_COMPLETE,
    SPI_STATE_ERROR
} SPI_State;

typedef struct SPI_Transaction SPI_Transaction;

struct SPI_Transaction {
    uint8_t bits;
    SPI_Baud baud;
    SPI_Operation operation;

    GPIO_TypeDef *cs_port;
    uint32_t cs_pin;
    bool cs_hold; // Hold pin if chained transaction uses the same pin.

    bool lsb_first;
    bool cke;
    bool ckp;

    void *tx_data;
    uint16_t tx_size;
    void *rx_data;
    uint16_t rx_size;
    SPI_State state;

    SPI_Transaction *(*callback)(SPI_Transaction *);

    void *callback_data;

    SPI_Transaction *queue_next;
};

void SPI_Init(void);

void SPI_Service(void);

void SPI_Queue(SPI_Transaction *tx);

#ifdef __cplusplus
}
#endif

#endif //SPI_H
