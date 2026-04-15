#include "csp.h"
#include "../../hal/pin.h"
#include "../../hal/polyfill/pic.h"
#include <pic18f57q84.h>
#include <xc.h>

void csp_init(csp_t *csp, pin_t rx, pin_t tx, pin_t de, uint8_t uart) {
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
    *pin_to_pps(de) = PPS_OUT_UART1_TXDE;
    U1RXPPS = rx;
    U1CTSPPS = PORT_E | PIN_7; // Non existant pin.
    U1CON1bits.ON = 1;
    break;
  case 2:
    U2CON0bits.MODE = 0b0000;
    U2CON0bits.TXEN = 1;
    U2CON0bits.RXEN = 1;
    U2BRGH = 0;
    U2BRGL = 39;
    *pin_to_pps(tx) = PPS_OUT_UART2_TX;
    *pin_to_pps(de) = PPS_OUT_UART2_TXDE;
    U2RXPPS = rx;
    U2CTSPPS = PORT_E | PIN_7; // Non existant pin.
    U2CON1bits.ON = 1;  
    break;
  case 3:
    U3CON0bits.MODE = 0b0000;
    U3CON0bits.TXEN = 1;
    U3CON0bits.RXEN = 1;
    U3BRGH = 0;
    U3BRGL = 39;
    *pin_to_pps(tx) = PPS_OUT_UART3_TX;
    *pin_to_pps(de) = PPS_OUT_UART3_TXDE;
    U3RXPPS = rx;
    U3CTSPPS = PORT_E | PIN_7; // Non existant pin.
    U3CON1bits.ON = 1;   
    break;
#ifdef U4TXB
  case 4:
    U4CON0bits.MODE = 0b0000;
    U4CON0bits.TXEN = 1;
    U4CON0bits.RXEN = 1;
    U4BRGH = 0;
    U4BRGL = 39;
    *pin_to_pps(tx) = PPS_OUT_UART4_TX;
    *pin_to_pps(de) = PPS_OUT_UART4_TXDE;
    U4RXPPS = rx;
    U4CTSPPS = PORT_E | PIN_7; // Non existant pin.
    U4CON1bits.ON = 1;   
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
    *pin_to_pps(de) = PPS_OUT_UART5_TXDE;
    U5RXPPS = rx;
    U5CTSPPS = PORT_E | PIN_7; // Non existant pin.
    U5CON1bits.ON = 1;   
    break;
#endif
  }
}

void csp_service(csp_t *csp) {}

void csp_interrupt_tx(csp_t *csp) {}

void csp_interrupt_rx(csp_t *csp) {}

void csp_set_addr(csp_t *csp, uint8_t addr) {}

uint8_t csp_get_rx_obj(csp_t *csp) { return CSP_NO_OBJECT; }

uint8_t csp_get_tx_obj(csp_t *csp) { return CSP_NO_OBJECT; }

void csp_send_tx_obj(csp_t *csp, uint8_t obj) {}

uint8_t *csp_get_object(csp_t *csp, uint8_t obj) { return NULL; }