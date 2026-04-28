#include "csp.h"
#include "../../hal/pin.h"
#include "../../hal/polyfill/pic.h"
#include "../../hal/interrupt.h"
#include "../../utils/mem.h"
#include <stdbool.h>
#include <xc.h>

void csp_set_tx_int(uint8_t uart, bool en);
uint8_t csp_find_free_buffer(csp_t *csp, uint8_t last);
bool csp_cobs_encode(uint8_t *dst, uint8_t *encoded_len, const uint8_t *src,
                     uint8_t src_len);
bool csp_cobs_decode(uint8_t *buffer, uint8_t *len);

void csp_init(csp_t *csp, pin_t rx, pin_t tx, pin_t de, uint8_t uart,
              uint8_t cfg, void (*callback)(csp_t *csp, uint8_t *ptr, uint8_t len)) {

  memset(csp, 0, sizeof(csp_t));

  csp->callback = callback;
  
  csp->uart = uart;
  csp->config = cfg;
  csp->address = CSP_ADDR_UNKNOWN;

  csp->rx.active = CSP_INACTIVE_OBJECT;
  csp->tx.active = CSP_INACTIVE_OBJECT;

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
bool csp_cobs_encode(uint8_t *dst, uint8_t *encoded_len, const uint8_t *src,
                     uint8_t src_len) {
  uint8_t read_index = 0;
  uint8_t write_index = 1;
  uint8_t code_index = 0;
  uint8_t code = 1;

  while (read_index < src_len) {
    uint8_t value = src[read_index];

    if (value == 0x00) {
      dst[code_index] = code;
      code_index = write_index;
      write_index++;
      code = 1;

      if (write_index > CSP_PAYLOAD_MAX) {
        return false;
      }
    } else {
      if (write_index >= CSP_PAYLOAD_MAX) {
        return false;
      }

      dst[write_index] = value;
      write_index++;
      code++;
    }

    read_index++;
  }

  dst[code_index] = code;
  *encoded_len = write_index;
  return true;
}

bool csp_cobs_decode(uint8_t *buffer, uint8_t *len) {
  uint8_t read_index = 0;
  uint8_t write_index = 0;
  uint8_t encoded_len = *len;

  while (read_index < encoded_len) {
    uint8_t code = buffer[read_index];
    if (code == 0x00) {
      return false;
    }

    read_index++;

    for (uint8_t i = 1; i < code; i++) {
      if (read_index >= encoded_len) {
        return false;
      }

      buffer[write_index] = buffer[read_index];
      write_index++;
      read_index++;
    }

    if ((code == 0xFF) && (read_index < encoded_len)) {
      return false;
    }

    if (read_index < encoded_len) {
      if (write_index >= CSP_PAYLOAD_MAX) {
        return false;
      }

      buffer[write_index] = 0x00;
      write_index++;
    }
  }

  *len = write_index;
  return true;
}

void csp_service(csp_t *csp) {
  uint8_t ready_rx = (~csp->buffer_type & csp->buffer_ready & csp->buffer_used);
  if (ready_rx != 0x00) {
    for (uint8_t i = 0; i < CSP_OBJECT_COUNT; i++) {
      uint8_t mask = (0x01 << i);
      if ((ready_rx & mask) != 0x00) {
        if (csp->buffer_size[i] == 0) {
          csp->errors.rx.runt++;
          continue;
        }

        if (!csp_cobs_decode(csp->buffer[i], &csp->buffer_size[i])) {
          csp->errors.rx.decode++;
          csp->buffer_used &= ~mask;
          csp->buffer_ready &= ~mask;
          continue;
        }

        if (csp->callback != NULL) {
          csp->callback(csp, &csp->buffer[i][0], csp->buffer_size[i]);
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
    data = csp->buffer[csp->tx.active][csp->tx.pos];
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
        csp->errors.rx.giant++;
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
          csp->buffer[csp->rx.active][csp->rx.pos] = data;
          csp->rx.pos++;
        }
      }
    } else {
      csp->errors.rx.overflow++;
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
    csp->errors.tx.giant++;
    return;
  }

  uint8_t bn = csp_find_free_buffer(csp, 0);
  if (bn == CSP_NO_OBJECT) {
    csp->errors.tx.overflow++;
    return;
  }

  uint8_t encoded_len = 0;
  if (!csp_cobs_encode(csp->buffer[bn], &encoded_len, ptr, len)) {
    csp->errors.tx.encode++;
    csp->buffer_used &= ~(0x01 << bn);
    return;
  }

  csp->buffer_size[bn] = encoded_len;

  uint8_t mask = 0x01 << bn;
  csp->buffer_type |= mask;
  csp->buffer_ready |= mask;
}

void csp_set_addr(csp_t *csp, uint8_t addr) {}
