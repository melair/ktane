#include "csp.h"
#include "../../hal/pin.h"
#include "../../hal/polyfill/pic.h"
#include "../../hal/interrupt.h"
#include <stdbool.h>
#include <xc.h>

void csp_set_tx_int(uint8_t uart, bool en);
uint8_t csp_find_free_buffer(csp_t *csp, uint8_t last);

void csp_init(csp_t *csp, pin_t rx, pin_t tx, pin_t de, uint8_t uart,
              uint8_t cfg) {

  csp->uart = uart;
  csp->config = cfg;
  csp->address = CSP_ADDR_UNKNOWN;

  csp->buffer_ready = 0x00;
  csp->buffer_type = 0x00;
  csp->buffer_used = 0x00;

  csp->rx.in_sync = 0;
  csp->rx.last = 0;
  csp->rx.active = CSP_INACTIVE_OBJECT;
  csp->rx.pos = 0;

  csp->tx.active = CSP_INACTIVE_OBJECT;
  csp->tx.pos = 0;

  csp->errors.rx_overflow = 0;
  csp->errors.rx_giant = 0;
  csp->errors.rx_runt = 0;
  csp->errors.tx_overflow = 0;
  csp->errors.tx_giant = 0;

  for (uint8_t i = 0; i < CSP_OBJECT_COUNT; i++) {
    csp->buffer_size[i] = 0x00;
  }

  pin_config(rx, INPUT, 0);
  pin_config(tx, OUTPUT, 0);

  if (de != PORTPIN_NONE) {
    pin_config(de, OUTPUT, 0);
  }

  switch (uart) {
  case 1:
    U1CON0bits.MODE = 0b0000;
    U1CON0bits.TXEN = 1;
    U1CON0bits.RXEN = 1;
    U1BRGH = 0;
    U1BRGL = 39;
    *pin_to_pps(tx) = PPS_OUT_UART1_TX;
    if (de != PORTPIN_NONE) {
      *pin_to_pps(de) = PPS_OUT_UART1_TXDE;
    }
    U1RXPPS = rx;
    U1CTSPPS = PORT_E | PIN_7; // Non existant pin.
    U1CON1bits.ON = 1;
    PIE4bits.U1RXIE = 1;
    break;
  case 2:
    U2CON0bits.MODE = 0b0000;
    U2CON0bits.TXEN = 1;
    U2CON0bits.RXEN = 1;
    U2BRGH = 0;
    U2BRGL = 39;
    *pin_to_pps(tx) = PPS_OUT_UART2_TX;
    if (de != PORTPIN_NONE) {
      *pin_to_pps(de) = PPS_OUT_UART2_TXDE;
    }
    U2RXPPS = rx;
    U2CTSPPS = PORT_E | PIN_7; // Non existant pin.
    U2CON1bits.ON = 1;
    PIE8bits.U2RXIE = 1;
    break;
  case 3:
    U3CON0bits.MODE = 0b0000;
    U3CON0bits.TXEN = 1;
    U3CON0bits.RXEN = 1;
    U3BRGH = 0;
    U3BRGL = 39;
    *pin_to_pps(tx) = PPS_OUT_UART3_TX;
    if (de != PORTPIN_NONE) {
      *pin_to_pps(de) = PPS_OUT_UART3_TXDE;
    }
    U3RXPPS = rx;
    U3CTSPPS = PORT_E | PIN_7; // Non existant pin.
    U3CON1bits.ON = 1;
    PIE9bits.U3RXIE = 1;
    break;
#ifdef U4TXB
  case 4:
    U4CON0bits.MODE = 0b0000;
    U4CON0bits.TXEN = 1;
    U4CON0bits.RXEN = 1;
    U4BRGH = 0;
    U4BRGL = 39;
    *pin_to_pps(tx) = PPS_OUT_UART4_TX;
    if (de != PORTPIN_NONE) {
      *pin_to_pps(de) = PPS_OUT_UART4_TXDE;
    }
    U4RXPPS = rx;
    U4CTSPPS = PORT_E | PIN_7; // Non existant pin.
    U4CON1bits.ON = 1;
    PIE12bits.U4RXIE = 1;
    break;
#endif
#ifdef U5TXB
  case 5:
    U5CON0bits.MODE = 0b0000;
    U5CON0bits.TXEN = 1;
    U5CON0bits.RXEN = 1;
    U5BRGH = 0;
    U5BRGL = 39;
    *pin_to_pps(tx) = PPS_OUT_UART5_TX;
    if (de != PORTPIN_NONE) {
      *pin_to_pps(de) = PPS_OUT_UART5_TXDE;
    }
    U5RXPPS = rx;
    U5CTSPPS = PORT_E | PIN_7; // Non existant pin.
    U5CON1bits.ON = 1;
    PIE13bits.U5RXIE = 1;
    break;
#endif
  }
}

void csp_service(csp_t *csp) {
  uint8_t ready_rx = (~csp->buffer_type & csp->buffer_ready & csp->buffer_used);
  if (ready_rx != 0x00) {
    for (uint8_t i = 0; i < CSP_OBJECT_COUNT; i++) {
      uint8_t mask = (0x01 << i);
      if ((ready_rx & mask) != 0x00) {
        if (csp->buffer_size[i] == 0) {
          csp->errors.rx_runt++;
          continue;
        }

        if (csp->callback != NULL) {
          csp->callback(csp, &csp->buffer[i * CSP_PAYLOAD_MAX],
                        csp->buffer_size[i]);
        }

        csp->buffer_used &= ~mask;
        csp->buffer_ready &= ~mask;
      }
    }
  }

  if (csp->tx.active == CSP_INACTIVE_OBJECT) {
    uint8_t ready_tx =
        (csp->buffer_type & csp->buffer_ready & csp->buffer_used);
    if (ready_tx != 0x00) {
      for (uint8_t i = 0; i < CSP_OBJECT_COUNT; i++) {
        uint8_t mask = (0x01 << i);
        if ((ready_tx & mask) != 0x00) {
          csp->tx.pos = 0;
          csp->tx.active = i;
          csp_set_tx_int(csp->uart, true);
          break;
        }
      }
    }
  }
}

void csp_interrupt_tx(csp_t *csp) {
  uint8_t data;

  if (csp->tx.active == CSP_INACTIVE_OBJECT) {
    csp_set_tx_int(csp->uart, false);
    return;
  }

  if (csp->tx.pos > csp->buffer_size[csp->tx.active]) {
    csp_set_tx_int(csp->uart, false);
    csp->buffer_used &= ~(0x01 << csp->tx.active);
    csp->buffer_ready &= ~(0x01 << csp->tx.active);
    csp->buffer_type &= ~(0x01 << csp->tx.active);
    csp->tx.active = CSP_INACTIVE_OBJECT;
    csp->tx.pos = 0x00;
    return;
  } else if (csp->tx.pos == csp->buffer_size[csp->tx.active]) {
    data = 0x00;
  } else {
    data = csp->buffer[(csp->tx.active * CSP_PAYLOAD_MAX) + csp->tx.pos];
  }

  csp->tx.pos++;

  switch (csp->uart) {
  case 1:
    U1TXB = data;
    break;
  case 2:
    U2TXB = data;
    break;
  case 3:
    U3TXB = data;
    break;
#ifdef U4TXB
  case 4:
    U4TXB = data;
    break;
#endif
#ifdef U5TXB
  case 5:
    U5TXB = data;
    break;
#endif
  default:
    return;
  }
}

void csp_interrupt_rx(csp_t *csp) {
  uint8_t data;

  switch (csp->uart) {
  case 1:
    data = U1RXB;
    break;
  case 2:
    data = U2RXB;
    break;
  case 3:
    data = U3RXB;
    break;
#ifdef U4RXB
  case 4:
    data = U4RXB;
    break;
#endif
#ifdef U5RXB
  case 5:
    data = U5RXB;
    break;
#endif
  default:
    return;
  }

  if (!csp->rx.in_sync) {
    if (data == 0x00) {
      csp->rx.in_sync = 1;
    }
  } else {
    if (csp->rx.active == CSP_INACTIVE_OBJECT) {
      uint8_t new = csp_find_free_buffer(csp, csp->rx.last);
      if (new != CSP_NO_OBJECT) {
        csp->rx.active = new;
        csp->rx.last = new;
        csp->rx.pos = 0;
        csp->buffer_type &= ~(0x01 << new);
        csp->buffer_ready &= ~(0x01 << new);
      }
    }

    if (csp->rx.active != CSP_INACTIVE_OBJECT) {
      if (csp->rx.pos >= CSP_PAYLOAD_MAX) {
        csp->errors.rx_giant++;
        csp->buffer_used &= ~(0x01 << csp->rx.active);
        csp->rx.active = CSP_INACTIVE_OBJECT;
        csp->rx.in_sync = 0;
      } else {
        if (data == 0x00) {
          csp->buffer_ready |= (0x01 << csp->rx.active);
          csp->buffer_size[csp->rx.active] = csp->rx.pos;
          csp->rx.pos = 0x00;
          csp->rx.active = CSP_INACTIVE_OBJECT;
        } else {
          csp->buffer[(csp->rx.active * CSP_PAYLOAD_MAX) + csp->rx.pos] = data;
          csp->rx.pos++;
        }
      }
    } else {
      csp->errors.rx_overflow++;
    }
  }
}

void csp_set_tx_int(uint8_t uart, bool en) {
  switch (uart) {
  case 1:
    PIE4bits.U1TXIE = (en ? 1 : 0);
    break;
  case 2:
    PIE8bits.U2TXIE = (en ? 1 : 0);
    break;
  case 3:
    PIE9bits.U3TXIE = (en ? 1 : 0);
    break;
#ifdef U4TXB
  case 4:
    PIE12bits.U4TXIE = (en ? 1 : 0);
    break;
#endif
#ifdef U5TXB
  case 5:
    PIE13bits.U5TXIE = (en ? 1 : 0);
    break;
#endif
  default:
    return;
  }
}

uint8_t csp_find_free_buffer(csp_t *csp, uint8_t last) {
  int_disable();
  uint8_t mask;

  for (uint8_t i = last; i < CSP_OBJECT_COUNT; i++) {
    mask = 0x01 << i;

    if ((csp->buffer_used & mask) == 0) {
      csp->buffer_used |= mask;
      int_enable();
      return i;
    }
  }

  for (uint8_t i = 0; i < last; i++) {
    mask = 0x01 << i;

    if ((csp->buffer_used & mask) == 0) {
      csp->buffer_used |= mask;
      int_enable();
      return i;
    }
  }

  int_enable();
  return CSP_NO_OBJECT;
}

void csp_tx(csp_t *csp, uint8_t *ptr, uint8_t len) {
  if (len > CSP_PAYLOAD_MAX) {
    csp->errors.tx_giant++;
    return;
  }

  uint8_t bn = csp_find_free_buffer(csp, 0);
  if (bn == CSP_NO_OBJECT) {
    csp->errors.tx_overflow++;
    return;
  }

  for (uint8_t i = 0; i < len; i++) {
    csp->buffer[(bn * CSP_PAYLOAD_MAX) + i] = ptr[i];
  }
  csp->buffer_size[bn] = len;

  uint8_t mask = 0x01 << bn;
  csp->buffer_type |= mask;
  csp->buffer_ready |= mask;
}

void csp_set_addr(csp_t *csp, uint8_t addr) {}