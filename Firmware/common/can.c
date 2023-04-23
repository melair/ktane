/**
 * Provide CAN functionality, used to communicate between the modules.
 */
#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include "can.h"
#include "packet.h"
#include "nvm.h"
#include "eeprom_addrs.h"
#ifdef SUPPORT_CAN_AUTO_ADDRESS
#include "../Application.X/can_auto_address.h"
#endif

/* CAN bus statistics. */
can_statistics_t can_stats;
/* Devices CAN ID. */
uint8_t can_identifier;
/* CAN details dirt. */
bool can_is_dirty;

/* Local function prototypes. */
bool can_change_mode(uint8_t mode);

/**
 * Initialise the CAN interface, used to communicate and coordinate modules.
 *
 * 500Kbs baud, 20 byte payload.
 */
void can_initialise(void) {
    /* Load last CAN id. */
    can_identifier = nvm_eeprom_read(EEPROM_LOC_CAN_ID);

#ifdef SUPPORT_CAN_AUTO_ADDRESS
    can_address_initialise(can_identifier);
#endif

    /* Configure CANTX/CANRX pins, RB0: RX, RB1: TX. */
    TRISBbits.TRISB0 = 1;
    TRISBbits.TRISB1 = 0;

    ANSELBbits.ANSELB0 = 0;
    ANSELBbits.ANSELB1 = 0;

    CANRXPPS = 0x08;
    RB1PPS = 0x46;

    /* Turn on CAN module. */
    C1CONHbits.ON = 1;

    /* Request CAN module to be in configuration mode. */
    if (!can_change_mode(0x04)) {
        return;
    }

    /* Initialise the FIFO start area, 0x3800 is the base of CAN specific
     * memory and does not consume any data SRAM. Data layout in data sheet
     * appears to be incorrect, it states it starts a 0x3C00 - MCC, forums and
     * the PIC18F57Q83 data sheet says it starts at 0x3800. */
    C1FIFOBA = 0x3800;

    /*** General CAN Configuration ***/

    /* Run from system clock. */
    C1CONLbits.CLKSEL0 = 0;

    /* Protocol exception event detection disabled, is enabled. */
    C1CONLbits.PXEDIS = 1;

    /* Use ISO CRC in CAN-FD frames. */
    C1CONLbits.ISOCRCEN = 1;

    /* Set data filter to 0 bytes for DeviceNet filter, i.e. do not filter on data bytes. */
    C1CONLbits.DNCNT = 0;

    /* Unknown bit, could be freeze in debug. */
    C1CONHbits.FRZ = 0;

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

    /* Do not limit retransmission attempts. */
    C1CONUbits.RTXAT = 0;

    /*** CAN Baud Configuration ***/

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

    /* Do not interrupt for transmission exhausted. */
    C1TXQCONLbits.TXATIE = 0;
    /* Do not interrupt for transmission queue being empty. */
    C1TXQCONLbits.TXQEIE = 0;
    /* Do not interrupt if transmission queue is not full. */
    C1TXQCONLbits.TXQNIE = 0;

    /* Clear the FIFO. */
    C1TXQCONHbits.FRESET = 1;
    /* Do not increment FIFO head. */
    C1TXQCONHbits.UINC = 0;

    /* Unlimited number of retries for transmission. */
    C1TXQCONUbits.TXAT = 3;
    /* Set transmission priority to 1. */
    C1TXQCONUbits.TXPRI = 1;

    /* CAN message payload size to 20 bytes. */
    C1TXQCONTbits.PLSIZE = 3;
    /* TX FIFO depth to 16. */
    C1TXQCONTbits.FSIZE = 0x10;

    /*** RX FIFO Configuration ***/

    /* Set FIFO to RX mode. */
    C1FIFOCON1Lbits.TXEN = 0;
    /* Disable auto remove transmit bit, not used for RX. */
    C1FIFOCON1Lbits.RTREN = 0;
    /* Do not capture receive timestamp. */
    C1FIFOCON1Lbits.RXTSEN = 0;
    /* Disable all interrupts. */
    C1FIFOCON1Lbits.TXATIE = 0;
    C1FIFOCON1Lbits.RXOVIE = 0;
    C1FIFOCON1Lbits.TFERFFIE = 0;
    C1FIFOCON1Lbits.TFHRFHIE = 0;
    C1FIFOCON1Lbits.TFNRFNIE = 0;

    /* Clear the FIFO. */
    C1FIFOCON1Hbits.FRESET = 1;
    /* Clear, no effect with RX FIFO. */
    C1FIFOCON1Hbits.TXREQ = 0;
    /* Disable FIFO tail increment. */
    C1FIFOCON1Hbits.UINC = 0;

    /* Unlimited retransmission, no effect with RX FIFO. */
    C1FIFOCON1Ubits.TXAT = 3;
    /* Transmission priority, no effect with RX FIFO. */
    C1FIFOCON1Ubits.TXPRI = 1;

    /* CAN message payload size to 20 bytes. */
    C1FIFOCON1Tbits.PLSIZE = 3;
    /* RX FIFO depth to 16. */
    C1FIFOCON1Tbits.FSIZE = 0x10;

    /* Request CAN module to be in normal CAN-FD mode. */
    if (!can_change_mode(0x00)) {
        return;
    }

    /*** Configure filter to allow receipt of all messages. ***/

    /* Place all matched messages in FIFO1. */
    C1FLTCON0Lbits.F0BP = 0x01;

    /* Match only standard length identifiers. */
    C1FLTOBJ0Tbits.EXIDE = 0;
    C1MASK0Tbits.MIDE = 1;

    /* Don't match on anything, should match all packets with a standard ID. */

    /* Enable filter. */
    C1FLTCON0Lbits.FLTEN0 = 1;
}

/**
 * Request and wait for CAN controller to change mode.
 *
 * @param mode new mode
 * @return true if mode change was successful
 */
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
            can_stats.can_error++;
            return false;
        }
    };

    return true;
}

static const uint8_t dlc_to_bytes[] = {0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 12U, 16U, 20U, 24U, 32U, 48U, 64U};

/**
 * Send packet out on CAN network.
 *
 * @param size size of bytes in data
 * @param data data payload to send
 */
void can_send(uint8_t prefix, uint8_t size, uint8_t *data) {
    /* Check to see if payload is too large. */
    if (size > 20) {
        can_stats.can_error++;
        return;
    }

    if (!can_ready() && prefix != PREFIX_NETWORK) {
        can_stats.tx_not_ready++;
        return;
    }

    /* Check to see if TX FIFO is full. */
    if (C1TXQSTALbits.TXQNIF == 0) {
        can_stats.tx_overflow++;
        return;
    }

    /* Cast tx buffer. */
    uint8_t *txbuffer = (uint8_t *) C1TXQUA;

    uint8_t dlc;
    for (dlc = 0; size > dlc_to_bytes[dlc]; dlc++);

    /* Set CAN header. */
    txbuffer[0] = can_identifier;
    txbuffer[1] = prefix & 0x07;
    txbuffer[2] = 0x00;
    txbuffer[3] = 0x00;
    txbuffer[4] = 0x80 | dlc; // FDF = 1
    txbuffer[5] = 0x00;
    txbuffer[6] = 0x00;
    txbuffer[7] = 0x00;

    /* Copy payload into CAN packet, ensure unused bytes are 0x00. */
    uint8_t i;

    for (i = 0; i < size && i < dlc_to_bytes[dlc]; i++) {
        txbuffer[8 + i] = data[i];
    }

    for (; i < dlc_to_bytes[dlc]; i++) {
        txbuffer[8 + i] = 0x00;
    }

    /* Request packet is sent, and increment FIFO head. */
    C1TXQCONHbits.UINC = 1;
    C1TXQCONHbits.TXREQ = 1;

    can_stats.tx_packets++;
}

/**
 * Service the CAN bus, such as receive messages.
 */
void can_service(void) {
    /* Check to see if the RX buffer is too full and has overflowed. */
    if (C1FIFOSTA1Lbits.RXOVIF == 1) {
        C1FIFOSTA1Lbits.RXOVIF = 0;
        can_stats.rx_overflow++;
    }

    /* Loop through the FIFO for new packets. */
    while (C1FIFOSTA1Lbits.TFNRFNIF == 1) {
        /* Process packet. */
        uint8_t *rxbuffer = (uint8_t *) C1FIFOUA1;

        /* Decode data. */
        uint8_t id = rxbuffer[0];
        uint8_t prefix = rxbuffer[1] & 0x07;

        can_stats.rx_packets++;

        /* Pass packet to protocol handling. */
        packet_route(id, prefix, (packet_t *) (&rxbuffer[8]));

        /* Increment the buffer. */
        C1FIFOCON1Hbits.UINC = 1;
    }

#ifdef SUPPORT_CAN_AUTO_ADDRESS
    can_address_service();
#endif
}

/**
 * Get the base CAN id of module.
 *
 * @return the base CAN id
 */
uint8_t can_get_id(void) {
    return can_identifier;
}

/**
 * Return true if the CAN bus is not ready, i.e. has no address id yet.
 *
 * @return true if read.
 */
bool can_ready(void) {
#ifdef SUPPORT_CAN_AUTO_ADDRESS
    return can_address_ready();
#else
    return true;
#endif
}

/**
 * Return the CAN statistics data.
 *
 * @return CAN statistics
 */
can_statistics_t *can_get_statistics(void) {
    return &can_stats;
}

bool can_dirty(void) {
    bool dirty = can_is_dirty;
    can_is_dirty = false;
    return dirty;
}
