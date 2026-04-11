#include "i2c.h"
#include "../utils/fsm.h"
#include "dma.h"
#include "i2c_internal.h"
#include "pin.h"
#include <xc.h>

/*
---
title: I2C
---
stateDiagram-v2
    [*] --> Idle
    Idle --> Dequeue
    Dequeue --> Configure
    Configure --> Write
    Configure --> Read
    Configure --> Callback
    Write --> Read
    Write --> Callback
    Read --> Callback
    Callback --> Configure
    Callback --> Idle
*/

extern const fs_t i2c_state_idle;
extern const fs_t i2c_state_dequeue;
extern const fs_t i2c_state_configure;
extern const fs_t i2c_state_write;
extern const fs_t i2c_state_read;
extern const fs_t i2c_state_callback;

i2c_t i2c;

void i2c_state_idle_service(fsm_t *fsm) {
  if (i2c.queue_head != NULL) {
    fsm_transition(fsm, &i2c_state_dequeue);
  }
}

const fs_t i2c_state_idle = {.name = "IDLE",
                             .next_states = {&i2c_state_dequeue, NULL},
                             .service = i2c_state_idle_service};

void i2c_state_dequeue_enter(fsm_t *fsm) {
  i2c.current = i2c.queue_head;
  i2c.queue_head = i2c.queue_head->queue_next;

  if (i2c.queue_tail == i2c.current) {
    i2c.queue_tail = NULL;
  }

  i2c.current->status = I2C_STATUS_SUCCESS;

  fsm_transition(fsm, &i2c_state_configure);
}

const fs_t i2c_state_dequeue = {.name = "DEQUEUE",
                                .next_states = {&i2c_state_configure, NULL},
                                .enter = i2c_state_dequeue_enter};

void i2c_state_configure_enter(fsm_t *fsm) {
  I2C1STAT1bits.CLRBF = 1;

  switch (i2c.current->operation) {
  case I2C_OPERATION_WRITE:
  case I2C_OPERATION_WRITE_STOP_READ:
  case I2C_OPERATION_WRITE_RESTART_READ:
    fsm_transition(fsm, &i2c_state_write);
    break;

  case I2C_OPERATION_READ:
    fsm_transition(fsm, &i2c_state_read);
    break;

  default:
    i2c.current->status = I2C_STATUS_ERROR_UNKNOWN;
    fsm_transition(fsm, &i2c_state_callback);
  }
}

const fs_t i2c_state_configure = {
    .name = "CONFIGURE",
    .next_states = {&i2c_state_write, &i2c_state_read, &i2c_state_callback, NULL},
    .enter = i2c_state_configure_enter};

void i2c_state_write_enter(fsm_t *fsm) {
  if (i2c.current->operation == I2C_OPERATION_WRITE_RESTART_READ) {
    I2C1CON0bits.RSEN = 1;
  } else {
    I2C1CON0bits.RSEN = 0;
  }

  i2c.RW_DONE = 0;

#ifdef I2C1CNT
  I2C1CNT = i2c.current->write_size;
#else
  I2C1CNTH = 0x00;
  I2C1CNTL = i2c.current->write_size & 0xff;
#endif

  I2C1ADB1 = i2c.current->addr & 0xfe;

  DMA_SELECT_BEGIN(I2C_DMA);

  DMAnCON0bits.EN = 0;

  DMAnCON1bits.SMODE = 0b01;
  DMAnCON1bits.DMODE = 0b00;

  DMAnSSZH = 0x00;
#ifdef I2C1CNT
  DMAnSSZL = I2C1CNT;
#else
  DMAnSSZL = I2C1CNTL;
#endif
  DMAnDSZ = 1;

  DMAnSSA = (volatile uint24_t)i2c.current->buffer;
  DMAnDSA = (volatile unsigned short)&I2C1TXB;

  DMAnCON1bits.SSTP = 1;
  DMAnCON1bits.DSTP = 0;

  DMAnSIRQ = 0x39; // I2C1TX Vector

  DMAnCON0bits.SIRQEN = 1;
  DMAnCON0bits.EN = 1;

  DMA_SELECT_END();

  I2C1CON0bits.S = 1;
}

void i2c_state_write_service(fsm_t *fsm) {
  if (i2c.current->status != I2C_STATUS_SUCCESS) {
    fsm_transition(fsm, &i2c_state_callback);
  }

  if (i2c.RW_DONE) {
    i2c.RW_DONE = 0;

    if (i2c.current->operation == I2C_OPERATION_WRITE) {
      fsm_transition(fsm, &i2c_state_callback);
    } else {
      fsm_transition(fsm, &i2c_state_read);
    }
  }
}

const fs_t i2c_state_write = {
    .name = "WRITE",
    .next_states = {&i2c_state_read, &i2c_state_callback, NULL},
    .enter = i2c_state_write_enter,
    .service = i2c_state_write_service};

void i2c_state_read_enter(fsm_t *fsm) {
  i2c.RW_DONE = 0;

#ifdef I2C1CNT
  I2C1CNT = i2c.current->read_size;
#else
  I2C1CNTH = 0x00;
  I2C1CNTL = i2c.current->read_size & 0xff;
#endif

  I2C1ADB1 = i2c.current->addr | 0x01;
  I2C1CON1bits.ACKCNT = 1;

  DMA_SELECT_BEGIN(I2C_DMA);

  DMAnCON0bits.EN = 0;

  DMAnCON1bits.SMODE = 0b00;
  DMAnCON1bits.DMODE = 0b01;

  DMAnSSZ = 1;
  DMAnDSZH = 0x00;
#ifdef I2C1CNT
  DMAnDSZL = I2C1CNT;
#else
  DMAnDSZL = I2C1CNTL;
#endif

  DMAnSSA = (volatile unsigned short)&I2C1RXB;
  DMAnDSA = (volatile uint24_t)i2c.current->buffer;

  DMAnCON1bits.SSTP = 0;
  DMAnCON1bits.DSTP = 1;

  DMAnSIRQ = 0x38; // I2C1RX Vector

  DMAnCON0bits.SIRQEN = 1;
  DMAnCON0bits.EN = 1;

  DMA_SELECT_END();

  I2C1CON0bits.S = 1;
}

void i2c_state_read_service(fsm_t *fsm) {
  if (i2c.current->status != I2C_STATUS_SUCCESS) {
    fsm_transition(fsm, &i2c_state_callback);
  }

  if (i2c.RW_DONE) {
    i2c.RW_DONE = 0;

    fsm_transition(fsm, &i2c_state_callback);
  }
}

void i2c_state_read_exit(fsm_t *fsm) { I2C1CON0bits.RSEN = 0; }

const fs_t i2c_state_read = {.name = "READ",
                             .next_states = {&i2c_state_callback, NULL},
                             .enter = i2c_state_read_enter,
                             .service = i2c_state_read_service,
                             .exit = i2c_state_read_exit};

void i2c_state_callback_enter(fsm_t *fsm) {
  DMA_SELECT_BEGIN(I2C_DMA);
  DMAnCON0bits.EN = 0;
  DMA_SELECT_END();

  if (i2c.current->callback != NULL) {
    i2c.current = i2c.current->callback(i2c.current);
  } else {
    i2c.current = NULL;
  }

  if (i2c.current == NULL) {
    fsm_transition(fsm, &i2c_state_idle);
  } else {
    fsm_transition(fsm, &i2c_state_configure);
  }
}

const fs_t i2c_state_callback = {
    .name = "CALLBACK",
    .next_states = {&i2c_state_configure, &i2c_state_idle, NULL},
    .enter = i2c_state_callback_enter};

void i2c_init(pin_t clk, pin_t dat) {
  /* Configure pins, all KTANE boards have external I2C pull ups, so don't
   * enable internal weak pull up. */
  pin_config(clk, OUTPUT, CFG_OPENDRAIN);
  pin_config(dat, OUTPUT, CFG_OPENDRAIN);

  I2C1SCLPPS = clk;
  I2C1SDAPPS = dat;

  *pin_to_pps(clk) = I2CCLKPPS;
  *pin_to_pps(dat) = I2CDATPPS;

  /* Configure I2C. */
  I2C1CON0bits.MODE = 0b100; // Host, 7-bit address mode
  I2C1CON2bits.ABD = 0;      // Use address buffer.

  /* Configure SCL frequency. */
  I2C1CON2bits.FME = 0; // Divider-by-5
  I2C1CLK = 0x03;       // MFINTOSC (500 kHz) / 5 = 100kHz
#ifdef I2C1BAUD
  /* Older PICs are more flexible with I2C baud rate, so need a BAUD value
     providing, more modern PICs assume you want standard frequencies, or you
     are willing to sacrifice a timer to achieve the same goal.

     Devices that support BAUD divide the CLK by (BAUD + 1) before the I2C
     divider controlled by FME, modern PICs don't support the initial division.
     */
  I2C1BAUD = 0x00; // Set BAUD to 0, which will result in I2C1CLK/1 before / 5.
#endif

#ifdef I2C1BTOC
  I2C1BTOC = 0x06;           // LFINTOSC as BTO clock source
  I2C1BTObits.TOREC = 1;     // Reset I2C interface, set BTOIF
  I2C1BTObits.TOBY32 = 0;    // BTO time = TOTIME * TBTOCLK
  I2C1BTObits.TOTIME = 0x0A; // TOTIME = TBTOCLK * 10 = 1 ms * 10 = 10 ms
#else
  T6INPPS = clk;
  T6CLK = 0x06;           // CLK = MFINTOSC (31.25KHz)
  T6CONbits.CKPS = 0x01;  // Pre-Scale = 2
  T6CONbits.OUTPS = 0x00; // Post-Scale = 0
  T6PR = 156;             // output = 31250/2/156 = 99.52 (10ms)
  T6HLT = 0x07;           // mode = free running with high level preset
  T6RST = 0x00;           // SCL1 (pin selected by T6INPPS)
  T6CONbits.ON = 0x01;

  I2C1BTO = 0x03;
#endif

  I2C1ERRbits.NACKIE = 1;
  I2C1ERRbits.BTOIE = 1;

  PIE7bits.I2C1IE = 1;
  PIE7bits.I2C1EIE = 1;
  I2C1PIEbits.CNTIE = 1;

  /* Enable I2C peripheral. */
  I2C1CON0bits.EN = 1;

  /* Ensure that I2C queues and pointers are empty. */
  i2c.queue_head = NULL;
  i2c.queue_tail = NULL;
  i2c.current = NULL;

  /* Initialise I2C FSM. */
  i2c.fsm.ctx = NULL;
  i2c.fsm.initial = &i2c_state_idle;
  fsm_init(&i2c.fsm);
}

void i2c_service(void) { fsm_service(&i2c.fsm); }

void i2c_interrupt(void) {
  if (PIR7bits.I2C1EIF) {
    if (I2C1ERRbits.BTOIF) {
      I2C1ERRbits.BTOIF = 0;

      if (i2c.current != NULL) {
        i2c.current->status = I2C_STATUS_ERROR_BTO;
      }
    }

    if (I2C1ERRbits.NACKIF) {
      I2C1ERRbits.NACKIF = 0;
      I2C1CON1bits.P = 1;

      if (i2c.current != NULL) {
        i2c.current->status = I2C_STATUS_ERROR_NACK;
      }
    }
  }

  if (I2C1PIRbits.CNTIF) {
    I2C1PIRbits.CNTIF = 0;

    if (i2c.current != NULL && i2c.current->operation != I2C_OPERATION_READ) {
      i2c.RW_DONE = 1;
    }
  }

  if (dma_intf_dcnt(I2C_DMA)) {
    if (i2c.current != NULL && i2c.current->operation == I2C_OPERATION_READ) {
      i2c.RW_DONE = 1;
    }
  }
}

void i2c_queue(i2c_transaction_t *transaction) {
  transaction->queue_next = NULL;

  if (i2c.queue_head == NULL) {
    i2c.queue_head = transaction;
  } else {
    i2c.queue_tail->queue_next = transaction;
  }

  i2c.queue_tail = transaction;
}
