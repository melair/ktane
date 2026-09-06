#include "fsm/fsm.h"
#include "i2c/i2c.h"
#include "i2c/i2c_platform.h"

typedef enum {
    I2C_FSM_STATE_IDLE = 0,
    I2C_FSM_STATE_DEQUEUE,
    I2C_FSM_STATE_CONFIGURE,
    I2C_FSM_STATE_WRITE,
    I2C_FSM_STATE_READ,
    I2C_FSM_STATE_CALLBACK
} I2C_FSM_State;

typedef struct {
    FSM fsm;

    I2C_Transaction *queue_head;
    I2C_Transaction *queue_tail;
    I2C_Transaction *current;

    volatile bool transfer_done;
    volatile bool transfer_error;
    volatile I2C_Status error_status;
} i2c_t;

static i2c_t i2c = {0};

static void i2c_fsm_idle_service(FSM *fsm);

static void i2c_fsm_dequeue_enter(FSM *fsm);

static void i2c_fsm_configure_enter(FSM *fsm);

static void i2c_fsm_write_enter(FSM *fsm);

static void i2c_fsm_write_service(FSM *fsm);

static void i2c_fsm_read_enter(FSM *fsm);

static void i2c_fsm_read_service(FSM *fsm);

static void i2c_fsm_callback_enter(FSM *fsm);

static const FSM_State i2c_fsm_states[] = {
    [I2C_FSM_STATE_IDLE] = {
        .service = i2c_fsm_idle_service,
        .next_mask = FSM_NEXT(I2C_FSM_STATE_DEQUEUE),
    },
    [I2C_FSM_STATE_DEQUEUE] = {
        .enter = i2c_fsm_dequeue_enter,
        .next_mask = FSM_NEXT(I2C_FSM_STATE_CONFIGURE),
    },
    [I2C_FSM_STATE_CONFIGURE] = {
        .enter = i2c_fsm_configure_enter,
        .next_mask = FSM_NEXT(I2C_FSM_STATE_WRITE) | FSM_NEXT(I2C_FSM_STATE_READ) | FSM_NEXT(I2C_FSM_STATE_CALLBACK),
    },
    [I2C_FSM_STATE_WRITE] = {
        .enter = i2c_fsm_write_enter,
        .service = i2c_fsm_write_service,
        .next_mask = FSM_NEXT(I2C_FSM_STATE_READ) | FSM_NEXT(I2C_FSM_STATE_CALLBACK),
    },
    [I2C_FSM_STATE_READ] = {
        .enter = i2c_fsm_read_enter,
        .service = i2c_fsm_read_service,
        .next_mask = FSM_NEXT(I2C_FSM_STATE_CALLBACK),
    },
    [I2C_FSM_STATE_CALLBACK] = {
        .enter = i2c_fsm_callback_enter,
        .next_mask = FSM_NEXT(I2C_FSM_STATE_IDLE) | FSM_NEXT(I2C_FSM_STATE_CONFIGURE),
    },
};

static uint16_t i2c_device_address(const I2C_Transaction *tx) {
    return (uint16_t) tx->address << 1U;
}

static bool start_write_phase(I2C_Transaction *tx) {
    if ((tx->tx_data == NULL) || (tx->tx_size == 0U)) {
        return false;
    }

    i2c.transfer_done = false;
    i2c.transfer_error = false;

    return I2C_PLATFORM.start_write(i2c_device_address(tx), tx->tx_data, tx->tx_size,
                                    tx->operation == I2C_OPERATION_WRITE_RESTART_READ);
}

static bool start_read_phase(I2C_Transaction *tx) {
    if ((tx->rx_data == NULL) || (tx->rx_size == 0U)) {
        return false;
    }

    i2c.transfer_done = false;
    i2c.transfer_error = false;

    return I2C_PLATFORM.start_read(i2c_device_address(tx), tx->rx_data, tx->rx_size,
                                   tx->operation == I2C_OPERATION_WRITE_RESTART_READ);
}

static I2C_Transaction *dequeue_transaction(void) {
    I2C_Transaction *transaction = i2c.queue_head;

    if (transaction != NULL) {
        i2c.queue_head = transaction->queue_next;
        if (i2c.queue_head == NULL) {
            i2c.queue_tail = NULL;
        }
        transaction->queue_next = NULL;
    }

    return transaction;
}

static void i2c_fsm_idle_service(FSM *fsm) {
    if (i2c.queue_head != NULL) {
        FSM_Transition(fsm, I2C_FSM_STATE_DEQUEUE);
    }
}

static void i2c_fsm_dequeue_enter(FSM *fsm) {
    i2c.current = dequeue_transaction();
    i2c.current->status = I2C_STATUS_SUCCESS;
    FSM_Transition(fsm, I2C_FSM_STATE_CONFIGURE);
}

static void i2c_fsm_configure_enter(FSM *fsm) {
    switch (i2c.current->operation) {
        case I2C_OPERATION_WRITE:
        case I2C_OPERATION_WRITE_STOP_READ:
        case I2C_OPERATION_WRITE_RESTART_READ:
            FSM_Transition(fsm, I2C_FSM_STATE_WRITE);
            break;
        case I2C_OPERATION_READ:
            FSM_Transition(fsm, I2C_FSM_STATE_READ);
            break;
        default:
            i2c.current->status = I2C_STATUS_ERROR_UNKNOWN;
            FSM_Transition(fsm, I2C_FSM_STATE_CALLBACK);
            break;
    }
}

static void i2c_fsm_write_enter(FSM *fsm) {
    if (!start_write_phase(i2c.current)) {
        i2c.current->status = I2C_STATUS_ERROR_UNKNOWN;
        FSM_Transition(fsm, I2C_FSM_STATE_CALLBACK);
    }
}

static void i2c_fsm_write_service(FSM *fsm) {
    if (i2c.transfer_error) {
        i2c.transfer_error = false;
        i2c.current->status = i2c.error_status;
        FSM_Transition(fsm, I2C_FSM_STATE_CALLBACK);
        return;
    }

    if (!i2c.transfer_done) {
        return;
    }

    i2c.transfer_done = false;

    if (i2c.current->operation == I2C_OPERATION_WRITE) {
        FSM_Transition(fsm, I2C_FSM_STATE_CALLBACK);
    } else {
        FSM_Transition(fsm, I2C_FSM_STATE_READ);
    }
}

static void i2c_fsm_read_enter(FSM *fsm) {
    if (!start_read_phase(i2c.current)) {
        i2c.current->status = I2C_STATUS_ERROR_UNKNOWN;
        FSM_Transition(fsm, I2C_FSM_STATE_CALLBACK);
    }
}

static void i2c_fsm_read_service(FSM *fsm) {
    if (i2c.transfer_error) {
        i2c.transfer_error = false;
        i2c.current->status = i2c.error_status;
        FSM_Transition(fsm, I2C_FSM_STATE_CALLBACK);
        return;
    }

    if (!i2c.transfer_done) {
        return;
    }

    i2c.transfer_done = false;
    FSM_Transition(fsm, I2C_FSM_STATE_CALLBACK);
}

static void i2c_fsm_callback_enter(FSM *fsm) {
    if (!I2C_PLATFORM.transfer_cleanup() && (i2c.current->status == I2C_STATUS_SUCCESS)) {
        i2c.current->status = I2C_STATUS_ERROR_UNKNOWN;
    }

    if (i2c.current->callback != NULL) {
        i2c.current = i2c.current->callback(i2c.current);
    } else {
        i2c.current = dequeue_transaction();
    }

    if (i2c.current != NULL) {
        i2c.current->queue_next = NULL;
        i2c.current->status = I2C_STATUS_SUCCESS;
        FSM_Transition(fsm, I2C_FSM_STATE_CONFIGURE);
    } else {
        FSM_Transition(fsm, I2C_FSM_STATE_IDLE);
    }
}

bool I2C_Init(void) {
    if (!I2C_PLATFORM.init(&I2C_HARDWARE)) {
        return false;
    }

    return FSM_Init(&i2c.fsm, i2c_fsm_states, I2C_FSM_STATE_IDLE, NULL);
}

void I2C_Service(void) {
    FSM_Service(&i2c.fsm);
}

void I2C_Queue(I2C_Transaction *tx) {
    if (tx == NULL) {
        return;
    }

    tx->queue_next = NULL;
    tx->status = I2C_STATUS_SUCCESS;

    if (i2c.queue_tail == NULL) {
        i2c.queue_head = tx;
        i2c.queue_tail = tx;
    } else {
        i2c.queue_tail->queue_next = tx;
        i2c.queue_tail = tx;
    }
}

void I2C_Platform_NotifyTransferComplete(void) {
    i2c.transfer_done = true;
}

void I2C_Platform_NotifyTransferError(I2C_Status status) {
    i2c.error_status = status;
    i2c.transfer_error = true;
}

void I2C_Platform_DMA_IRQHandler(void) {
    I2C_PLATFORM.dma_irq();
}

void I2C_Platform_Event_IRQHandler(void) {
    I2C_PLATFORM.event_irq();
}

void I2C_Platform_Error_IRQHandler(void) {
    I2C_PLATFORM.error_irq();
}
