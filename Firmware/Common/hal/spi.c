#include "spi.h"
#include "../utils/fsm.h"
#include "../utils/time.h"
#include "dma.h"
#include "mcu.h"
#include "pin.h"
#include "spi_internal.h"
#include "time.h"
#include <stdint.h>
#include <xc.h>

/*
---
title: SPI
---
stateDiagram-v2
    [*] --> Idle
    Idle --> Dequeue
    Dequeue --> Configure
    Configure --> CSAssert
    CSAssert --> Ready
    CSAssert -->|Wait| Ready
    Ready --> Read
    Ready --> Write
    Write --> Read
    Write --> Callback
    Read --> Callback
    Callback --> Configure
    Callback --> CSDeassert
    CSDeassert --> Configure
    CSDeassert --> Idle
*/

extern const fs_t state_idle;
extern const fs_t state_dequeue;
extern const fs_t state_configure;
extern const fs_t state_cs_assert;
extern const fs_t state_ready;
extern const fs_t state_write;
extern const fs_t state_read;
extern const fs_t state_callback;
extern const fs_t state_cs_deassert;

spi_t spi;

void spi_state_idle_service(fsm_t *fsm) {
  if (spi.queue_head != NULL) {
    fsm_transition(fsm, &state_dequeue);
  }
}

const fs_t state_idle = {.name = "IDLE",
                         .next_states = {&state_dequeue, NULL},
                         .service = spi_state_idle_service};

void spi_state_dequeue_enter(fsm_t *fsm) {
  spi.current = spi.queue_head;
  spi.queue_head = spi.queue_head->queue_next;

  if (spi.queue_tail == spi.current) {
    spi.queue_tail = NULL;
  }

  fsm_transition(fsm, &state_configure);
}

const fs_t state_dequeue = {.name = "DEQUEUE",
                            .next_states = {&state_configure, NULL},
                            .enter = spi_state_dequeue_enter};

void spi_state_configure_enter(fsm_t *fsm) {
  SPIBAUD = spi.current->baud;

  if (spi.current->lsb_first) {
    SPICON0 |= _SPI1CON0_LSBF_MASK;
  } else {
    SPICON0 &= ~_SPI1CON0_LSBF_MASK;
  }

  if (spi.current->cke) {
    SPICON1 |= _SPI1CON1_CKE_MASK;
  } else {
    SPICON1 &= ~_SPI1CON1_CKE_MASK;
  }

    if (spi.current->bits == 8) {
      SPITWIDTH = 8;
    } else {
      SPITWIDTH = spi.current->bits;
    }

  fsm_transition(fsm, &state_cs_assert);
}

const fs_t state_configure = {.name = "CONFIGURE",
                              .next_states = {&state_cs_assert, NULL},
                              .enter = spi_state_configure_enter};

void spi_state_cs_assert_enter(fsm_t *fsm) {
  spi.current_cs_pin = spi.current->cs_pin;
  pin_write(spi.current_cs_pin, false);

  if (spi.current->cs_wait_ms != 0) {
    fsm_transition_in(fsm, &state_ready, spi.current->cs_wait_ms);
  } else {
    fsm_transition(fsm, &state_ready);
  }
}

const fs_t state_cs_assert = {
    .name = "CS ASSERT",
    .next_states = {&state_ready, NULL},
    .enter = spi_state_cs_assert_enter};

void spi_state_ready_enter(fsm_t *fsm) {
  switch (spi.current->operation) {
  case SPI_OPERATION_WRITE:
  case SPI_OPERATION_WRITE_THEN_READ:
    fsm_transition(fsm, &state_write);
    break;

  case SPI_OPERATION_READ:
    fsm_transition(fsm, &state_read);
    break;
  }
}

const fs_t state_ready = {.name = "READY",
                          .next_states = {&state_write, &state_read, NULL},
                          .enter = spi_state_ready_enter};

/* SPI writing is complete when the SPI peripheral is idle. */

void spi_state_write_enter(fsm_t *fsm) {
  SPICON2 &= ~_SPI1CON2_RXR_MASK;
  SPICON2 |= _SPI1CON2_TXR_MASK;
  SPISTATUS |= _SPI1STATUS_CLRBF_MASK;

  SPITCNTH = (spi.current->write_size >> 8) & 0x07;
  SPITCNTL = (spi.current->write_size) & 0xff;

  DMASELECT = SPI_DMA;

  DMAnCON1bits.SMODE = 0b01;
  DMAnCON1bits.DMODE = 0b00;

  DMAnSSZH = SPITCNTH;
  DMAnSSZL = SPITCNTL;
  DMAnDSZ = 1;

  DMAnSSA = (volatile uint24_t)spi.current->buffer;
  DMAnDSA = (volatile unsigned short)&SPITXB;

  DMAnCON1bits.SSTP = 1;
  DMAnCON1bits.DSTP = 0;

  DMAnSIRQ = SPITXVECTOR;

  spi.RW_DONE = 0;

  SPICON0 |= _SPI1CON0_EN_MASK;

  DMAnCON0bits.SIRQEN = 1;
  DMAnCON0bits.EN = 1;
}

void spi_state_write_service(fsm_t *fsm) {
  if (spi.RW_DONE) {
    spi.RW_DONE = 0;

    if (spi.current->operation == SPI_OPERATION_WRITE_THEN_READ) {
      fsm_transition(fsm, &state_read);
    } else {
      fsm_transition(fsm, &state_callback);
    }
  }
}

const fs_t state_write = {.name = "WRITE",
                          .next_states = {&state_read, &state_callback, NULL},
                          .enter = spi_state_write_enter,
                          .service = spi_state_write_service};

/* SPI reading is complete when the DMA peripheral has copied the last received
 * byte, not when the peripheral is idle. */

void spi_state_read_enter(fsm_t *fsm) {
  SPICON2 |= _SPI1CON2_RXR_MASK;
  SPICON2 &= ~_SPI1CON2_TXR_MASK;
  SPISTATUS |= _SPI1STATUS_CLRBF_MASK;

  SPITCNTH = (spi.current->read_size >> 8) & 0x07;
  SPITCNTL = (spi.current->read_size) & 0xff;

  DMASELECT = SPI_DMA;

  DMAnCON1bits.SMODE = 0b00;
  DMAnCON1bits.DMODE = 0b01;

  DMAnSSZ = 1;
  DMAnDSZH = SPITCNTH;
  DMAnDSZL = SPITCNTL;

  DMAnSSA = (volatile unsigned short)&SPIRXB;
  DMAnDSA = (volatile uint24_t)spi.current->buffer;

  DMAnCON1bits.SSTP = 0;
  DMAnCON1bits.DSTP = 1;

  DMAnSIRQ = SPIRXVECTOR;

  spi.RW_DONE = 0;

  SPICON0 |= _SPI1CON0_EN_MASK;

  DMAnCON0bits.SIRQEN = 1;
  DMAnCON0bits.EN = 1;
}

void spi_state_read_service(fsm_t *fsm) {
  if (spi.RW_DONE) {
    spi.RW_DONE = 0;

    fsm_transition(fsm, &state_callback);
  }
}

const fs_t state_read = {.name = "READ",
                         .next_states = {&state_callback, NULL},
                         .enter = spi_state_read_enter,
                         .service = spi_state_read_service};

void spi_state_callback_enter(fsm_t *fsm) {
  SPICON0 &= ~_SPI1CON0_EN_MASK;
  DMASELECT = SPI_DMA;
  DMAnCON0bits.EN = 0;

  if (spi.current->callback != NULL) {
    spi.current = spi.current->callback(spi.current);

    if (spi.current_cs_pin == spi.current->cs_pin &&
        !spi.current->cs_bounce) {
      fsm_transition(fsm, &state_configure);
    } else {
      fsm_transition(fsm, &state_cs_deassert);
    }

    return;
  }

  fsm_transition(fsm, &state_cs_deassert);
}

const fs_t state_callback = {
    .name = "CALLBACK",
    .next_states = {&state_cs_deassert, &state_configure, NULL},
    .enter = spi_state_callback_enter};

void spi_stat_cs_deassert_enter(fsm_t *fsm) {
  pin_write(spi.current_cs_pin, true);

  if (spi.current != NULL) {
    fsm_transition(fsm, &state_configure);
  } else {
    fsm_transition(fsm, &state_idle);
  }
}

const fs_t state_cs_deassert = {
    .name = "CS DEASSERT",
    .next_states = {&state_idle, &state_configure, NULL},
    .enter = spi_stat_cs_deassert_enter};

void spi_init(pin_t copi, pin_t clk, pin_t cipo) {
  /* Set clock to FOSC. */
  SPICLK = 0x00;

  /* Set to SPI controller. */
  SPICON0 |= _SPI1CON0_MST_MASK;

  /* Set to bit mode, and set width to be 8. */
  SPICON0 |= _SPI1CON0_BMODE_MASK;
  SPITWIDTH &= ~_SPI1TWIDTH_TWIDTH_MASK;

  /* Enable zero count interrupt. */
  SPIINTE |= _SPI1INTE_TCZIE_MASK;

  *pin_to_pps(copi) = SPIPPSCOPI;
  *pin_to_pps(clk) = SPIPPSCLK;

  SPIIE = 1;
  SPISDIPPS = cipo;

  dma_inte_dcnt(SPI_DMA, true);

  /* Ensure that SPI queues and pointers are empty. */
  spi.queue_head = NULL;
  spi.queue_tail = NULL;
  spi.current = NULL;

  /* Initialise SPI FSM. */
  spi.fsm.ctx = NULL;
  spi.fsm.initial = &state_idle;
  fsm_init(&spi.fsm);
}

void spi_service(void) { fsm_service(&spi.fsm); }

void spi_interrupt(void) {
  DMA_SELECT_BEGIN(SPI_DMA);

  if ((DMAnCON0 & _DMAnCON0_DGO_MASK) != _DMAnCON0_DGO_MASK &&
      (SPICON2 & _SPI1CON2_BUSY_MASK) != _SPI1CON2_BUSY_MASK &&
      (SPIINTF & _SPI1INTF_TCZIF_MASK) == _SPI1INTF_TCZIF_MASK) {
    SPIINTF &= ~_SPI1INTF_TCZIF_MASK;

    if (spi.current->operation == SPI_OPERATION_WRITE ||
        spi.current->operation == SPI_OPERATION_WRITE_THEN_READ) {

      if (spi.current->write_repeats > 0) {
        spi.current->write_repeats--;

        SPICON0 &= ~_SPI1CON0_EN_MASK;

        SPITCNTH = (spi.current->write_size >> 8) & 0x07;
        SPITCNTL = (spi.current->write_size) & 0xff;

        SPISTATUS |= _SPI1STATUS_CLRBF_MASK;

        DMAnCON0bits.EN = 0;

        SPICON0 |= _SPI1CON0_EN_MASK;
        DMAnCON0bits.SIRQEN = 1;
        DMAnCON0bits.EN = 1;
      } else {
        spi.RW_DONE = 1;
      }
    }
  }

  if (dma_intf_dcnt(SPI_DMA)) {
    if (spi.current->operation == SPI_OPERATION_READ) {
      spi.RW_DONE = 1;
    }
  }

  DMA_SELECT_END();
}

void spi_queue(spi_transaction_t *transaction) {
  transaction->queue_next = NULL;

  if (spi.queue_head == NULL) {
    spi.queue_head = transaction;
  } else {
    spi.queue_tail->queue_next = transaction;
  }

  spi.queue_tail = transaction;
}