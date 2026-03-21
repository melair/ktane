#include "spi.h"
#include "../utils/fsm.h"
#include "dma.h"
#include "pin.h"
#include <stdint.h>
#include <xc.h>

static const spi_peripheral_t spi_peripherals[] = {{
                                                       .num = SPI1,
                                                       .clk = &SPI1CLK,
                                                       .con0 = &SPI1CON0,
                                                       .con1 = &SPI1CON1,
                                                       .con2 = &SPI1CON2,
                                                       .twidth = &SPI1TWIDTH,
                                                       .inte = &SPI1INTE,
                                                       .intf = &SPI1INTF,
                                                       .baud = &SPI1BAUD,
                                                       .status = &SPI1STATUS,
                                                       .tcntl = &SPI1TCNTL,
                                                       .tcnth = &SPI1TCNTH,
                                                       .txb = &SPI1TXB,
                                                       .rxb = &SPI1RXB,

                                                       .spitx_vector = 0x19,
                                                       .spirx_vector = 0x18,
                                                   }
#ifdef SPI2CON0
                                                   ,
                                                   {
                                                       .num = SPI2,
                                                       .clk = &SPI2CLK,
                                                       .con0 = &SPI2CON0,
                                                       .con1 = &SPI2CON1,
                                                       .con2 = &SPI2CON2,
                                                       .twidth = &SPI2TWIDTH,
                                                       .inte = &SPI2INTE,
                                                       .intf = &SPI2INTF,
                                                       .baud = &SPI2BAUD,
                                                       .status = &SPI2STATUS,
                                                       .tcntl = &SPI2TCNTL,
                                                       .tcnth = &SPI2TCNTH,
                                                       .txb = &SPI2TXB,
                                                       .rxb = &SPI2RXB,

                                                       .spitx_vector = 0x29,
                                                       .spirx_vector = 0x28,
                                                   }
#endif
};

/*
---
title: SPI
---
stateDiagram-v2
    [*] --> Idle
    Idle --> Dequeue
    Dequeue --> Configure
    Configure --> CSAssert
    CSAssert --> CSWait
    CSAssert --> Ready
    CSWait --> Ready
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

const fs_t state_idle;
const fs_t state_dequeue;
const fs_t state_configure;
const fs_t state_cs_assert;
const fs_t state_cs_wait;
const fs_t state_ready;
const fs_t state_write;
const fs_t state_read;
const fs_t state_callback;
const fs_t state_cs_deassert;

void spi_state_idle_service(fsm_t *fsm) {
  spi_t *spi = (spi_t *)fsm->ctx;

  if (spi->queue_head != NULL) {
    fsm_transition(fsm, &state_dequeue);
  }
}

const fs_t state_idle = {.name = "IDLE",
                         .next_states = {&state_dequeue, NULL},
                         .service = spi_state_idle_service};

void spi_state_dequeue_enter(fsm_t *fsm) {
  spi_t *spi = (spi_t *)fsm->ctx;

  spi->current = spi->queue_head;
  spi->queue_head = spi->queue_head->queue_next;

  fsm_transition(fsm, &state_configure);
}

const fs_t state_dequeue = {.name = "DEQUEUE",
                            .next_states = {&state_configure, NULL},
                            .enter = spi_state_dequeue_enter};

void spi_state_configure_enter(fsm_t *fsm) {
  spi_t *spi = (spi_t *)fsm->ctx;

  uint32_t baud = ((64000000 / spi->current->baud) / 2) - 1;
  *spi->spi_peripheral->baud = (uint8_t) baud;

  if (spi->current->lsb_first) {
    *spi->spi_peripheral->con0 |= _SPI1CON0_LSBF_MASK;
  } else {
    *spi->spi_peripheral->con0 &= ~_SPI1CON0_LSBF_MASK;
  }

  if (spi->current->cke) {
    *spi->spi_peripheral->con1 |= _SPI1CON1_CKE_MASK;
  } else {
    *spi->spi_peripheral->con1 &= ~_SPI1CON1_CKE_MASK;
  }

  *spi->spi_peripheral->twidth =
      (spi->current->bits == 8 ? 0 : spi->current->bits);

  fsm_transition(fsm, &state_cs_assert);
}

const fs_t state_configure = {.name = "CONFIGURE",
                              .next_states = {&state_cs_assert, NULL},
                              .enter = spi_state_configure_enter};

void spi_state_cs_assert_enter(fsm_t *fsm) {
  spi_t *spi = (spi_t *)fsm->ctx;

  spi->current_cs_pin = spi->current->cs_pin;
  pin_write(spi->current_cs_pin, false);

  if (spi->current->cs_wait_ms != 0) {
    fsm_transition(fsm, &state_cs_wait);
  } else {
    fsm_transition(fsm, &state_ready);
  }
}

const fs_t state_cs_assert = {
    .name = "CS ASSERT",
    .next_states = {&state_cs_wait, &state_ready, NULL},
    .enter = spi_state_cs_assert_enter};

void spi_state_cs_wait_service(fsm_t *fsm) {
  spi_t *spi = (spi_t *)fsm->ctx;

  // TODO: INCOMPLETE - NEEDS CLOCK!

  fsm_transition(fsm, &state_ready);
}

const fs_t state_cs_wait = {.name = "CS WAIT",
                            .next_states = {&state_ready, NULL},
                            .service = spi_state_cs_wait_service};

void spi_state_ready_enter(fsm_t *fsm) {
  spi_t *spi = (spi_t *)fsm->ctx;

  switch (spi->current->operation) {
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
  spi_t *spi = (spi_t *)fsm->ctx;

  *spi->spi_peripheral->con2 &= ~_SPI1CON2_RXR_MASK;
  *spi->spi_peripheral->con2 |= _SPI1CON2_TXR_MASK;
  *spi->spi_peripheral->status |= _SPI1STATUS_CLRBF_MASK;

  *spi->spi_peripheral->tcnth = (spi->current->write_size >> 8) & 0x07;
  *spi->spi_peripheral->tcntl = (spi->current->write_size) & 0xff;

  DMASELECT = spi->dma_peripheral;

  DMAnCON1bits.SMODE = 0b01;
  DMAnCON1bits.DMODE = 0b00;

  DMAnSSZH = *spi->spi_peripheral->tcnth;
  DMAnSSZL = *spi->spi_peripheral->tcntl;
  DMAnDSZ = 1;

  DMAnSSA = (volatile uint24_t)spi->current->buffer;
  DMAnDSA = (volatile unsigned short)spi->spi_peripheral->txb;

  DMAnCON1bits.SSTP = 1;
  DMAnCON1bits.DSTP = 0;

  DMAnSIRQ = spi->spi_peripheral->spitx_vector;

  // TODO: Needed?
  dma_intf_dcnt(spi->dma_peripheral);

  *spi->spi_peripheral->con0 |= _SPI1CON0_EN_MASK;

  DMAnCON0bits.SIRQEN = 1;
  DMAnCON0bits.EN = 1;
}

void spi_state_write_service(fsm_t *fsm) {
  spi_t *spi = (spi_t *)fsm->ctx;

  if ((*spi->spi_peripheral->con2 & _SPI1CON2_BUSY_MASK) == 0 &&
      (*spi->spi_peripheral->intf & _SPI1INTF_TCZIF_MASK) ==
          _SPI1INTF_TCZIF_MASK) {
    *spi->spi_peripheral->intf &= ~_SPI1INTF_TCZIF_MASK;

    if (spi->current->operation == SPI_OPERATION_WRITE_THEN_READ) {
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
  spi_t *spi = (spi_t *)fsm->ctx;

  *spi->spi_peripheral->con2 |= _SPI1CON2_RXR_MASK;
  *spi->spi_peripheral->con2 &= ~_SPI1CON2_TXR_MASK;
  *spi->spi_peripheral->status |= _SPI1STATUS_CLRBF_MASK;

  *spi->spi_peripheral->tcnth = (spi->current->read_size >> 8) & 0x07;
  *spi->spi_peripheral->tcntl = (spi->current->read_size) & 0xff;

  DMASELECT = spi->dma_peripheral;

  DMAnCON1bits.SMODE = 0b00;
  DMAnCON1bits.DMODE = 0b01;

  DMAnSSZ = 1;
  DMAnDSZH = *spi->spi_peripheral->tcnth;
  DMAnDSZL = *spi->spi_peripheral->tcntl;

  DMAnSSA = (volatile uint24_t)spi->current->buffer;
  DMAnDSA = (volatile unsigned short)spi->spi_peripheral->rxb;

  DMAnCON1bits.SSTP = 0;
  DMAnCON1bits.DSTP = 1;

  DMAnSIRQ = spi->spi_peripheral->spirx_vector;

  dma_intf_dcnt(spi->dma_peripheral);

  *spi->spi_peripheral->con0 |= _SPI1CON0_EN_MASK;

  DMAnCON0bits.SIRQEN = 1;
  DMAnCON0bits.EN = 1;
}

void spi_state_read_service(fsm_t *fsm) {
  spi_t *spi = (spi_t *)fsm->ctx;

  if (dma_intf_dcnt(spi->dma_peripheral)) {
    fsm_transition(fsm, &state_callback);
  }
}

const fs_t state_read = {.name = "READ",
                         .next_states = {&state_callback, NULL},
                         .enter = spi_state_read_enter,
                         .service = spi_state_read_service};

void spi_state_callback_enter(fsm_t *fsm) {
  spi_t *spi = (spi_t *)fsm->ctx;

  *spi->spi_peripheral->con0 &= ~_SPI1CON0_EN_MASK;
  DMASELECT = spi->dma_peripheral;
  DMAnCON0bits.EN = 0;

  if (spi->current->callback != NULL) {
    spi->current = spi->current->callback(spi->current);

    if (spi->current != NULL) {
      if (spi->current_cs_pin == spi->current->cs_pin &&
          !spi->current->cs_bounce) {
        fsm_transition(fsm, &state_configure);
      } else {
        fsm_transition(fsm, &state_cs_deassert);
      }

      return;
    }
  }

  fsm_transition(fsm, &state_cs_deassert);
}

const fs_t state_callback = {
    .name = "CALLBACK",
    .next_states = {&state_cs_deassert, &state_configure, NULL},
    .enter = spi_state_callback_enter};

void spi_stat_cs_deassert_enter(fsm_t *fsm) {
  spi_t *spi = (spi_t *)fsm->ctx;

  pin_write(spi->current_cs_pin, true);

  if (spi->current != NULL) {
    fsm_transition(fsm, &state_configure);
  } else {
    fsm_transition(fsm, &state_idle);
  }
}

const fs_t state_cs_deassert = {
    .name = "CS DEASSERT",
    .next_states = {&state_idle, &state_configure, NULL},
    .enter = spi_stat_cs_deassert_enter};

void spi_init(spi_t *spi, pin_t copi, pin_t clk, pin_t cipo, uint8_t config) {
  spi->spi_peripheral = &spi_peripherals[SPI_NUM(config)];
  spi->dma_peripheral = DMA_NUM(config);

  /* Set clock to FOSC. */
  *spi->spi_peripheral->clk = 0x00;

  /* Set to SPI controller. */
  *spi->spi_peripheral->con0 |= _SPI1CON0_MST_MASK;

  /* Set to bit mode, and set width to be 8. */
  *spi->spi_peripheral->con0 |= _SPI1CON0_BMODE_MASK;
  *spi->spi_peripheral->twidth &= ~_SPI1TWIDTH_TWIDTH_MASK;

  /* Enable zero count interrupt. */
  *spi->spi_peripheral->inte |= _SPI1INTE_TCZIE_MASK;

  switch (spi->spi_peripheral->num) {
  case SPI1:
#if defined(_PIC18F15Q40_H_)
    *pin_to_pps(copi) = 0x1C;
    *pin_to_pps(clk) = 0x1B;
#elif defined(_PIC18F57Q84_H_)
    *pin_to_pps(copi) = 0x32;
    *pin_to_pps(clk) = 0x31;
#else
#error Unsupported PIC, requires PPS values.
#endif
    SPI1SDIPPS = cipo;
    PIE3bits.SPI1IE = 1;
    break;
  case SPI2:
#if defined(_PIC18F15Q40_H_)
    *pin_to_pps(copi) = 0x1F;
    *pin_to_pps(clk) = 0x1E;
#elif defined(_PIC18F57Q84_H_)
    *pin_to_pps(copi) = 0x35;
    *pin_to_pps(clk) = 0x34;
#else
#error Unsupported PIC, requires PPS values.
#endif
    SPI2SDIPPS = cipo;
    PIE5bits.SPI2IE = 1;
    break;
  }

  dma_inte_dcnt(spi->dma_peripheral, true);

  /* Ensure that SPI queues and pointers are empty. */
  spi->queue_head = NULL;
  spi->queue_tail = NULL;
  spi->current = NULL;

  /* Initialise SPI FSM. */
  spi->fsm.ctx = spi;
  spi->fsm.initial = &state_idle;
  fsm_init(&spi->fsm);
}

void spi_service(spi_t *spi) { fsm_service(&spi->fsm); }

void spi_interrupt(spi_t *spi) {}

void spi_queue(spi_t *spi, spi_transaction_t *transaction) {
  transaction->queue_next = NULL;

  if (spi->queue_head == NULL) {
    spi->queue_head = transaction;
  } else {
    spi->queue_tail->queue_next = transaction;
  }

  spi->queue_tail = transaction;
}