#ifndef I2C_H
#define I2C_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {

#endif

typedef enum {
    I2C_OPERATION_WRITE = 0,
    I2C_OPERATION_WRITE_STOP_READ,
    I2C_OPERATION_WRITE_RESTART_READ,
    I2C_OPERATION_READ,
} I2C_Operation;

typedef enum {
    I2C_STATUS_SUCCESS = 0,
    I2C_STATUS_ERROR_BTO,
    I2C_STATUS_ERROR_NACK,
    I2C_STATUS_ERROR_UNKNOWN,
} I2C_Status;

typedef struct I2C_Transaction I2C_Transaction;

struct I2C_Transaction {
    I2C_Operation operation;
    volatile I2C_Status status;

    uint8_t address;

    uint8_t *tx_data;
    uint16_t tx_size;
    uint8_t *rx_data;
    uint16_t rx_size;

    I2C_Transaction *(*callback)(I2C_Transaction *);

    void *callback_data;

    I2C_Transaction *queue_next;
};

void I2C_Init(void);

void I2C_Service(void);

void I2C_Queue(I2C_Transaction *tx);

#ifdef __cplusplus
}
#endif

#endif //I2C_H
