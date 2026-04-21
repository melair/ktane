#include "can.h"
#include "../utils/time.h"
#include "pin.h"
#include "polyfill/pic.h"
#include <xc.h>

bool can_change_mode(uint8_t mode);

typedef struct {
  pin_t act_led;
  bool act_led_on;

  struct {
    uint16_t error;

    struct {
        uint16_t overflow;
        uint16_t received;
    } rx;

    struct {
        uint16_t overflow;
        uint16_t giant;
        uint16_t failure;
        uint16_t queued;
    } tx;
  } stats;

  void (*callback)(uint16_t id, uint8_t len, uint8_t *ptr);
} can_t;

can_t can;

void can_init(pin_t tx, pin_t rx, pin_t act,
              void (*callback)(uint16_t id, uint8_t len, uint8_t *ptr)) {
  can.act_led = act;
  can.callback = callback;

  if (act != PORTPIN_NONE) {
    pin_config(act, OUTPUT, CFG_OPENDRAIN);
    pin_write(act, true);
  }

  pin_config(tx, OUTPUT, 0);
  pin_config(rx, INPUT, 0);

  *pin_to_pps(tx) = PPS_OUT_CANTX;
  CANRXPPS = rx;

  /* Turn CAN module on. */
  C1CONHbits.ON = 1;

  if (!can_change_mode(CAN_MODE_CONFIG)) {
    return;
  }

  /* Set base of FIFO. */
  C1FIFOBA = 0x3800;

  /* Run from system clock. */
  C1CONLbits.CLKSEL0 = 0;

  /* Protocol exception event detection disabled, is enabled. */
  C1CONLbits.PXEDIS = 1;

  /* Use ISO CRC in CAN-FD frames. */
  C1CONLbits.ISOCRCEN = 1;

  /* Set data filter to 0 bytes for DeviceNet filter, i.e. do not filter on data
   * bytes. */
  C1CONLbits.DNCNT = 0;

  /* Don't stop can in idle mode. */
  C1CONHbits.SIDL = 0;

  /* Disable bit rate switching. */
  C1CONHbits.BRSDIS = 1;

  /* Enable wake up filter, and set to T11. */
  C1CONHbits.WFT = 3;
  C1CONHbits.WAKFIL = 1;

  /* Enable transmission queue. */
  C1CONUbits.TXQEN = 1;

  /* Do not store transmitted messages in TEF. */
  C1CONUbits.STEF = 0;

  /* Do not move to listen only on system error, instead restricted operation. */
  C1CONUbits.SERR2LOM = 0;

  /* Set ESI bit to be error status of CAN controller. */
  C1CONUbits.ESIGM = 0;

  /* Do limit retransmission attempts. */
  C1CONUbits.RTXAT = 1;

  /* Configure bit rate timings for 500kbs CAN-FD, calculated by MCC. */
  /* SJW = 25 */
  C1NBTCFGL = 0x19;
  /* TSEG2 = 25*/
  C1NBTCFGH = 0x19;
  /* TSEG1 = 100*/
  C1NBTCFGU = 0x64;
  /* BRP = 0 */
  C1NBTCFGT = 0x00;

  /*** TX FIFO Configuration ***/
  /* Interrupt for transmission attempts exhausted. */
  C1TXQCONLbits.TXATIE = 1;
  /* Do not interrupt for transmission queue being empty. */
  C1TXQCONLbits.TXQEIE = 0;
  /* Do not interrupt if transmission queue is not full. */
  C1TXQCONLbits.TXQNIE = 0;

  /* Clear the FIFO. */
  C1TXQCONHbits.FRESET = 1;
  /* Do not increment FIFO head. */
  C1TXQCONHbits.UINC = 0;

  /* Limit to 3 retries for transmission. */
  C1TXQCONUbits.TXAT = 0b01;
  /* Set transmission priority to 1. */
  C1TXQCONUbits.TXPRI = 1;

  /* CAN message max payload size to 64 bytes. */
  C1TXQCONTbits.PLSIZE = 0b111;
  /* TX FIFO depth to 16. */
  C1TXQCONTbits.FSIZE = 0x10;

  /*** RX FIFO Configuration ***/
  /* Set FIFO to RX mode. */
  C1FIFOCON1Lbits.TXEN = 0;
  /* Disable auto remove transmit bit. */
  C1FIFOCON1Lbits.RTREN = 0;
  /* Do not capture receive timestamp. */
  C1FIFOCON1Lbits.RXTSEN = 0;
  /* Disable all interrupts. */
  C1FIFOCON1Lbits.TXATIE = 0;
  C1FIFOCON1Lbits.TFERFFIE = 0;
  C1FIFOCON1Lbits.TFHRFHIE = 0;
  C1FIFOCON1Lbits.TFNRFNIE = 0;
  /* Interrupt for the RX overflow. */
  C1FIFOCON1Lbits.RXOVIE = 0;

  /* Clear the FIFO. */
  C1FIFOCON1Hbits.FRESET = 1;
  /* Clear, no effect with RX FIFO. */
  C1FIFOCON1Hbits.TXREQ = 0;
  /* Disable FIFO tail increment. */
  C1FIFOCON1Hbits.UINC = 0;

  /* Unlimited retransmission, no effect with RX FIFO. */
  C1FIFOCON1Ubits.TXAT = 0;
  /* Transmission priority, no effect with RX FIFO. */
  C1FIFOCON1Ubits.TXPRI = 0;

  /* CAN message max payload size to 64 bytes. */
  C1FIFOCON1Tbits.PLSIZE = 0b111;
  /* RX FIFO depth to 16. */
  C1FIFOCON1Tbits.FSIZE = 0x10;

  /* Enable interrupts. */
  C1INTTbits.RXOVIE = 1;
  C1INTTbits.TXATIE = 1;
  C1INTTbits.WAKIE = 1;

  /* Start CAN module. */
  if (!can_change_mode(CAN_MODE_NORMAL_CAN_FD)) {
    return;
  }

  /* Enable system error interrupt. */
  C1INTTbits.SERRIE = 1;

  /*** Configure filter to allow receipt of all messages. ***/
  /* Place all matched messages in FIFO1. */
  C1FLTCON0Lbits.F0BP = 0x01;

  /* Match only standard length identifiers. */
  C1FLTOBJ0Tbits.EXIDE = 0;
  C1MASK0Tbits.MIDE = 1;

  /* Enable filter, without specifying filter, to catch all. */
  C1FLTCON0Lbits.FLTEN0 = 1;
}

bool can_change_mode(uint8_t mode) {
  /* Mask mode bits only. */
  uint8_t requested_mode = mode & 0x07;

  /* Request CAN module to be in configuration mode. */
  C1CONTbits.REQOP = requested_mode;

  /* Wait until CAN module moves to configuration mode, though should be
   * in configuration at power up. */
  while (C1CONUbits.OPMOD != requested_mode) {
    /* If a system error occurred increase the statistic and return false. */
    if (C1INTHbits.SERRIF == 1) {
      C1INTHbits.SERRIF = 0;
      can.stats.error++;
      return false;
    }
  }

  return true;
}

static const uint8_t dlc_to_bytes[] = {0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 12U, 16U, 20U, 24U, 32U, 48U, 64U};

void can_tx(uint16_t can_id, uint8_t len, uint8_t *ptr) {
    /* Check size isn't beyond CAN. */
    if (len > 64) {
        can.stats.tx.giant++;
        return;
    }

    /* Check to see if TX FIFO is full. */
    if (C1TXQSTALbits.TXQNIF == 0) {
        can.stats.tx.overflow++;
        return;
    }

    /* Get TX buffer. */
    uint8_t *txbuffer = (uint8_t *) C1TXQUA;

    /* Calculate wire size. */
    uint8_t dlc;
    for (dlc = 0; len > dlc_to_bytes[dlc]; dlc++);

    /* Set CAN header. */
    txbuffer[0] = can_id & 0xff;
    txbuffer[1] = (can_id >> 8) & 0x07;
    txbuffer[2] = 0x00;
    txbuffer[3] = 0x00;
    txbuffer[4] = 0x80 | (dlc & 0x0f); // FDF = 1, CAN-FD frame.
    txbuffer[5] = 0x00;
    txbuffer[6] = 0x00;
    txbuffer[7] = 0x00;

    /* Copy payload into CAN packet, ensure unused bytes are 0x00. */
    uint8_t i;

    for (i = 0; i < len; i++) {
        txbuffer[8 + i] = ptr[i];
    }

    for (; i < dlc_to_bytes[dlc]; i++) {
        txbuffer[8 + i] = 0x00;
    }

    /* Request packet is sent, and increment FIFO head. */
    C1TXQCONHbits.UINC = 1;
    C1TXQCONHbits.TXREQ = 1;

    /* Turn on TX activity light. */
    if (!can.act_led_on) {
        can.act_led_on = true;
        pin_write(can.act_led, false);
    }

    can.stats.tx.queued++;
}

void can_service(void) {
    /* If the TX light is on, and the queue is empty, turn it off. */
    if (C1TXQSTALbits.TXQEIF == 1 && can.act_led_on) {
        pin_write(can.act_led, true);
        can.act_led_on = false;
    }

    /* Loop through the FIFO for new packets. */
    while (C1FIFOSTA1Lbits.TFNRFNIF == 1) {
        can.stats.rx.received++;

        /* Process packet. */
        uint8_t *rxbuffer = (uint8_t *) C1FIFOUA1;

        /* Decode data. */
        uint16_t can_id = rxbuffer[0] | ((rxbuffer[1] & 0x07) << 8);
        uint8_t dlc = rxbuffer[5] & 0x0f;   
        uint8_t len = dlc_to_bytes[dlc];

        /* Pass packet to protocol handling. */
        if (can.callback != NULL) {
            can.callback(can_id, len, &rxbuffer[8]);
        }
        
        /* Increment the buffer. */
        C1FIFOCON1Hbits.UINC = 1;
    }
}

void can_interrupt(void) {
    /* Check to see if the RX buffer is too full and has overflowed. */
    if (C1FIFOSTA1Lbits.RXOVIF == 1) {
        C1FIFOSTA1Lbits.RXOVIF = 0;
        can.stats.rx.overflow++;
    }

    /* Check to see if the TX attempts are exceeded. */
    if (C1TXQSTALbits.TXATIF == 1) {
        C1TXQSTALbits.TXATIF = 0;
        can.stats.tx.failure++;
    }

    /* CAN system error. */
    if (C1INTHbits.SERRIF == 1) {
        C1INTHbits.SERRIF = 0;
        can.stats.error++;
    }

    /* Wake the MCU. */
    if (C1INTHbits.WAKIF == 1) {
        C1INTHbits.WAKIF = 0;
    }
}