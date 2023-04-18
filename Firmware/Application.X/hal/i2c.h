#ifndef I2C_H
#define	I2C_H

#include <stdint.h>

#define I2C_OPERATION_WRITE             0
#define I2C_OPERATION_WRITE_THEN_READ   1
#define I2C_OPERATION_READ              2

typedef struct i2c_command_t i2c_command_t;

#define I2C_STATE_IN_PROGRESS   0
#define I2C_STATE_SUCCESS       1
#define I2C_STATE_ERROR         2

struct i2c_command_t {
    unsigned operation      :2;
    unsigned in_progress    :1;
    unsigned state          :2;

    uint8_t addr;
    
    uint8_t *buffer;
    uint16_t write_size;
    uint16_t read_size;
        
    i2c_command_t *(*callback)(i2c_command_t *);
    void *callback_ptr;
    
    i2c_command_t *next_cmd;
};

void i2c_initialise(void);
void i2c_service(void);
void i2c_enqueue(i2c_command_t *c);
i2c_command_t *i2c_unused_callback(i2c_command_t *cmd);

#endif	/* I2C_H */

