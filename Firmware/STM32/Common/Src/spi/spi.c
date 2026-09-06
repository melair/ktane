#include "fsm/fsm.h"
#include "spi/spi.h"
#include "spi/spi_platform.h"

typedef enum {
    SPI_FSM_STATE_IDLE = 0,
    SPI_FSM_STATE_DEQUEUE,
    SPI_FSM_STATE_CS_ASSERT,
    SPI_FSM_STATE_READY,
    SPI_FSM_STATE_TX,
    SPI_FSM_STATE_RX,
    SPI_FSM_STATE_CALLBACK,
    SPI_FSM_STATE_CS_DEASSERT
} SPI_FSM_State;

typedef struct {
    FSM fsm;

    SPI_Transaction *queue_head;
    SPI_Transaction *queue_tail;
    SPI_Transaction *active_transaction;
    SPI_Transaction *next_transaction;

    bool active_cs_asserted;
    volatile bool transfer_complete;
    volatile bool transfer_error;
} spi_t;

static spi_t spi = {0};

static void spi_fsm_idle_service(FSM *fsm);

static void spi_fsm_dequeue_enter(FSM *fsm);

static void spi_fsm_cs_assert_enter(FSM *fsm);

static void spi_fsm_ready_enter(FSM *fsm);

static void spi_fsm_tx_enter(FSM *fsm);

static void spi_fsm_tx_service(FSM *fsm);

static void spi_fsm_rx_enter(FSM *fsm);

static void spi_fsm_rx_service(FSM *fsm);

static void spi_fsm_callback_enter(FSM *fsm);

static void spi_fsm_cs_deassert_enter(FSM *fsm);

static const FSM_State spi_fsm_states[] = {
    [SPI_FSM_STATE_IDLE] = {
        .service = spi_fsm_idle_service,
        .next_mask = FSM_NEXT(SPI_FSM_STATE_DEQUEUE),
    },
    [SPI_FSM_STATE_DEQUEUE] = {
        .enter = spi_fsm_dequeue_enter,
        .next_mask = FSM_NEXT(SPI_FSM_STATE_IDLE) | FSM_NEXT(SPI_FSM_STATE_CS_ASSERT),
    },
    [SPI_FSM_STATE_CS_ASSERT] = {
        .enter = spi_fsm_cs_assert_enter,
        .next_mask = FSM_NEXT(SPI_FSM_STATE_READY) | FSM_NEXT(SPI_FSM_STATE_CALLBACK),
    },
    [SPI_FSM_STATE_READY] = {
        .enter = spi_fsm_ready_enter,
        .next_mask = FSM_NEXT(SPI_FSM_STATE_TX) | FSM_NEXT(SPI_FSM_STATE_RX) | FSM_NEXT(SPI_FSM_STATE_CALLBACK),
    },
    [SPI_FSM_STATE_TX] = {
        .enter = spi_fsm_tx_enter,
        .service = spi_fsm_tx_service,
        .next_mask = FSM_NEXT(SPI_FSM_STATE_RX) | FSM_NEXT(SPI_FSM_STATE_CALLBACK),
    },
    [SPI_FSM_STATE_RX] = {
        .enter = spi_fsm_rx_enter,
        .service = spi_fsm_rx_service,
        .next_mask = FSM_NEXT(SPI_FSM_STATE_CALLBACK),
    },
    [SPI_FSM_STATE_CALLBACK] = {
        .enter = spi_fsm_callback_enter,
        .next_mask = FSM_NEXT(SPI_FSM_STATE_IDLE) | FSM_NEXT(SPI_FSM_STATE_CS_DEASSERT),
    },
    [SPI_FSM_STATE_CS_DEASSERT] = {
        .enter = spi_fsm_cs_deassert_enter,
        .next_mask = FSM_NEXT(SPI_FSM_STATE_IDLE) | FSM_NEXT(SPI_FSM_STATE_CS_ASSERT),
    },
};

static bool configure_spi(const SPI_Transaction *tx) {
    return SPI_PLATFORM.configure(tx->bits, tx->baud, tx->lsb_first, tx->cke, tx->ckp);
}

static bool start_tx_phase(SPI_Transaction *tx) {
    if ((tx->tx_data == NULL) || (tx->tx_size == 0U) || !configure_spi(tx)) {
        return false;
    }

    spi.transfer_complete = false;
    spi.transfer_error = false;
    tx->state = SPI_STATE_TX;

    return SPI_PLATFORM.start_write(tx->tx_data, tx->tx_size, tx->bits);
}

static bool start_rx_phase(SPI_Transaction *tx) {
    if ((tx->rx_data == NULL) || (tx->rx_size == 0U) || !configure_spi(tx)) {
        return false;
    }

    spi.transfer_complete = false;
    spi.transfer_error = false;
    tx->state = SPI_STATE_RX;

    return SPI_PLATFORM.start_read(tx->rx_data, tx->rx_size, tx->bits);
}

static SPI_Transaction *dequeue_transaction(void) {
    SPI_Transaction *transaction = spi.queue_head;

    if (transaction != NULL) {
        spi.queue_head = transaction->queue_next;
        if (spi.queue_head == NULL) {
            spi.queue_tail = NULL;
        }
        transaction->queue_next = NULL;
    }

    return transaction;
}

static void spi_fsm_idle_service(FSM *fsm) {
    if (spi.queue_head != NULL) {
        FSM_Transition(fsm, SPI_FSM_STATE_DEQUEUE);
    }
}

static void spi_fsm_dequeue_enter(FSM *fsm) {
    spi.active_transaction = dequeue_transaction();
    spi.active_cs_asserted = false;

    if (spi.active_transaction == NULL) {
        FSM_Transition(fsm, SPI_FSM_STATE_IDLE);
        return;
    }

    spi.active_transaction->state = SPI_STATE_IDLE;
    FSM_Transition(fsm, SPI_FSM_STATE_CS_ASSERT);
}

static void spi_fsm_cs_assert_enter(FSM *fsm) {
    if ((spi.active_transaction->cs_port == NULL) || (spi.active_transaction->cs_pin == 0U)) {
        spi.active_transaction->state = SPI_STATE_ERROR;
        FSM_Transition(fsm, SPI_FSM_STATE_CALLBACK);
        return;
    }

    SPI_PLATFORM.chip_select(spi.active_transaction->cs_port, spi.active_transaction->cs_pin, true);
    spi.active_cs_asserted = true;
    FSM_Transition(fsm, SPI_FSM_STATE_READY);
}

static void spi_fsm_ready_enter(FSM *fsm) {
    switch (spi.active_transaction->operation) {
        case SPI_OPERATION_WRITE:
        case SPI_OPERATION_WRITE_THEN_READ:
            FSM_Transition(fsm, SPI_FSM_STATE_TX);
            break;
        case SPI_OPERATION_READ:
            FSM_Transition(fsm, SPI_FSM_STATE_RX);
            break;
        default:
            spi.active_transaction->state = SPI_STATE_ERROR;
            FSM_Transition(fsm, SPI_FSM_STATE_CALLBACK);
            break;
    }
}

static void spi_fsm_tx_enter(FSM *fsm) {
    if (!start_tx_phase(spi.active_transaction)) {
        spi.active_transaction->state = SPI_STATE_ERROR;
        FSM_Transition(fsm, SPI_FSM_STATE_CALLBACK);
    }
}

static void spi_fsm_tx_service(FSM *fsm) {
    if (spi.transfer_error) {
        spi.transfer_error = false;
        spi.active_transaction->state = SPI_STATE_ERROR;
        FSM_Transition(fsm, SPI_FSM_STATE_CALLBACK);
        return;
    }

    if (!spi.transfer_complete) {
        return;
    }

    spi.transfer_complete = false;

    if (spi.active_transaction->operation == SPI_OPERATION_WRITE_THEN_READ) {
        FSM_Transition(fsm, SPI_FSM_STATE_RX);
    } else {
        FSM_Transition(fsm, SPI_FSM_STATE_CALLBACK);
    }
}

static void spi_fsm_rx_enter(FSM *fsm) {
    if (!start_rx_phase(spi.active_transaction)) {
        spi.active_transaction->state = SPI_STATE_ERROR;
        FSM_Transition(fsm, SPI_FSM_STATE_CALLBACK);
    }
}

static void spi_fsm_rx_service(FSM *fsm) {
    if (spi.transfer_error) {
        spi.transfer_error = false;
        spi.active_transaction->state = SPI_STATE_ERROR;
        FSM_Transition(fsm, SPI_FSM_STATE_CALLBACK);
        return;
    }

    if (!spi.transfer_complete) {
        return;
    }

    spi.transfer_complete = false;
    FSM_Transition(fsm, SPI_FSM_STATE_CALLBACK);
}

static void spi_fsm_callback_enter(FSM *fsm) {
    spi.next_transaction = NULL;

    if (spi.active_transaction == NULL) {
        FSM_Transition(fsm, SPI_FSM_STATE_IDLE);
        return;
    }

    if (!SPI_PLATFORM.transfer_cleanup()) {
        spi.active_transaction->state = SPI_STATE_ERROR;
    } else if (spi.active_transaction->state != SPI_STATE_ERROR) {
        spi.active_transaction->state = SPI_STATE_COMPLETE;
    }

    if (spi.active_transaction->callback != NULL) {
        spi.next_transaction = spi.active_transaction->callback(spi.active_transaction);
    }

    if (spi.next_transaction == NULL) {
        spi.next_transaction = dequeue_transaction();
    }

    FSM_Transition(fsm, SPI_FSM_STATE_CS_DEASSERT);
}

static void spi_fsm_cs_deassert_enter(FSM *fsm) {
    if ((spi.active_transaction != NULL) && spi.active_cs_asserted &&
        (!spi.active_transaction->cs_hold || (spi.next_transaction == NULL) ||
         (spi.active_transaction->cs_port != spi.next_transaction->cs_port) ||
         (spi.active_transaction->cs_pin != spi.next_transaction->cs_pin))) {
        SPI_PLATFORM.chip_select(spi.active_transaction->cs_port, spi.active_transaction->cs_pin, false);
    }

    spi.active_transaction = spi.next_transaction;
    spi.next_transaction = NULL;
    spi.active_cs_asserted = false;

    if (spi.active_transaction != NULL) {
        spi.active_transaction->queue_next = NULL;
        spi.active_transaction->state = SPI_STATE_IDLE;
        FSM_Transition(fsm, SPI_FSM_STATE_CS_ASSERT);
    } else {
        FSM_Transition(fsm, SPI_FSM_STATE_IDLE);
    }
}

bool SPI_Init(void) {
    if (!SPI_PLATFORM.init(&SPI_HARDWARE)) {
        return false;
    }

    return FSM_Init(&spi.fsm, spi_fsm_states, SPI_FSM_STATE_IDLE, NULL);
}

void SPI_Service(void) {
    FSM_Service(&spi.fsm);
}

void SPI_Queue(SPI_Transaction *tx) {
    if (tx == NULL) {
        return;
    }

    tx->queue_next = NULL;
    tx->state = SPI_STATE_IDLE;

    if (spi.queue_tail == NULL) {
        spi.queue_head = tx;
        spi.queue_tail = tx;
    } else {
        spi.queue_tail->queue_next = tx;
        spi.queue_tail = tx;
    }
}

void SPI_Platform_NotifyTransferComplete(void) {
    spi.transfer_complete = true;
}

void SPI_Platform_NotifyTransferError(void) {
    spi.transfer_error = true;
}

void SPI_Platform_DMA_IRQHandler(void) {
    SPI_PLATFORM.dma_irq();
}

void SPI_Platform_SPI_IRQHandler(void) {
    SPI_PLATFORM.spi_irq();
}
