#ifndef I2C_H
#define I2C_H

#include "../utils/fsm.h"
#include "pin.h"
#include <stdint.h>

typedef struct i2c_transaction_t i2c_transaction_t;
typedef struct i2c_t i2c_t;

struct i2c_t {
  fsm_t fsm;

  i2c_transaction_t *queue_head;
  i2c_transaction_t *queue_tail;

  i2c_transaction_t *current;

  unsigned RW_DONE : 1;
};

#define I2C_OPERATION_WRITE 0
#define I2C_OPERATION_WRITE_STOP_READ 1
#define I2C_OPERATION_WRITE_RESTART_READ 2
#define I2C_OPERATION_READ 3

#define I2C_STATUS_SUCCESS 0
#define I2C_STATUS_ERROR_BTO 1
#define I2C_STATUS_ERROR_NACK 2

struct i2c_transaction_t {
  unsigned operation : 3;
  unsigned status : 2;

  uint8_t addr;

  uint8_t *buffer;
  uint8_t write_size;
  uint8_t read_size;

  i2c_transaction_t *(*callback)(i2c_transaction_t *);
  void *callback_data;

  i2c_transaction_t *queue_next;
};

void i2c_init(pin_t clk, pin_t dat);
void i2c_service(void);
void i2c_interrupt(void);
void i2c_queue(i2c_transaction_t *transaction);

#endif