/**
 * Provide CAN functionality, used to communicate between the modules.
 */
#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include "can.h"
#include "nvm.h"
#include "argb.h"
#include "protocol.h"
#include "protocol_network.h"
#include "serial.h"
#include "rng.h"
#include "tick.h"
#include "module.h"

#define CAN_ADDRESS_RNG_MASK 0x74926411
#define CAN_ADDRESS_CHECKS 4

/* CAN bus statistics. */
can_statistics_t stats;
/* Devices CAN ID. */
uint8_t can_identifier;
/* Devices CAN domain. */
uint8_t can_domain;

/* CAN address seed. */
uint32_t can_address_seed;
uint8_t can_address_phase;
uint32_t can_address_clear_tick;
uint8_t can_address_eeprom;

/* Local function prototypes. */
bool can_change_mode(uint8_t mode);
void can_address_service(void);

/**
 * Initialise the CAN interface, used to communicate and coordinate modules.
 *
 * 500Kbs baud, 20 byte payload.
 */
void can_initialise(void) {
    /* Set our address to no address. */
    can_identifier = CAN_NO_ADDRESS;

    /* Retrieve the CAN EEPROM address. */
    can_address_eeprom = nvm_read(EEPROM_LOC_CAN_ID);
    if (can_address_eeprom == 0x00) {
        can_address_eeprom = CAN_NO_ADDRESS;
    }

    /* Load last CAN domain. */
    can_domain = nvm_read(EEPROM_LOC_CAN_DOMAIN);

    /* Set our address seed, our serial number. This should reduce/eliminate
     * conflicts. */
    can_address_seed = serial_get();
    can_address_phase = 0;

    /* Configure CANTX/CANRX pins, RB0: RX, RB1: TX. */
    TRISBbits.TRISB0 = 1;
    TRISBbits.TRISB1 = 0;

    ANSELBbits.ANSELB0 = 0;
    ANSELBbits.ANSELB1 = 0;

    CANRXPPS = 0b00001000;
    RB1PPS = 0x46;

    /* Turn on CAN module. */
    C1CONHbits.ON = 1;

    /* Request CAN module to be in configuration mode. */
    if (!can_change_mode(0b100)) {
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
    C1CONHbits.WFT = 0b11;
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
    C1TXQCONTbits.PLSIZE = 0b011;
    /* TX FIFO depth to 16. */
    C1TXQCONTbits.FSIZE = 0b10000;

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
    C1FIFOCON1Ubits.TXAT = 0b11;
    /* Transmission priority, no effect with RX FIFO. */
    C1FIFOCON1Ubits.TXPRI = 1;

    /* CAN message payload size to 20 bytes. */
    C1FIFOCON1Tbits.PLSIZE = 0b011;
    /* RX FIFO depth to 16. */
    C1FIFOCON1Tbits.FSIZE = 0b10000;

    /* Request CAN module to be in normal CAN-FD mode. */
    if (!can_change_mode(0b000)) {
        return;
    }

    /*** Configure filter to allow receipt of all messages. ***/

    /* Place all matched messages in FIFO1. */
    C1FLTCON0Lbits.F0BP = 0b00001;

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
    uint8_t requested_mode = mode & 0b111;

    /* Request CAN module to be in configuration mode. */
    C1CONTbits.REQOP = requested_mode;

    /* Wait until CAN module moves to configuration mode, though should be
     * in configuration at power up. */
    while (C1CONUbits.OPMOD != requested_mode) {
        /* If a system error occurred increase the statistic and return false. */
        if (C1INTHbits.SERRIF == 1) {
            stats.can_error++;
            return false;
        }
    };

    return true;
}

/**
 * Send packet out on CAN network.
 *
 * @param size size of bytes in data
 * @param data data payload to send
 */
void can_send(uint8_t prefix, uint8_t size, uint8_t *data) {
    /* Check to see if payload is too large. */
    if (size > 20) {
        return;
    }

    if (!can_ready() && prefix != PREFIX_NETWORK) {
        stats.tx_not_ready++;
        return;
    }

    /* Check to see if TX FIFO is full. */
    if (C1TXQSTALbits.TXQNIF == 0) {
        stats.tx_overflow++;
        return;
    }

    /* Cast tx buffer. */
    uint8_t *txbuffer = (uint8_t *) C1TXQUA;

    /* Set CAN header. */
    txbuffer[0] = can_identifier;
    txbuffer[1] = prefix & 0b00000111;
    txbuffer[2] = 0x00;
    txbuffer[3] = 0x00;
    txbuffer[4] = 0b10001011; // FDF = 1, DLC = 11 (20 byte payload)
    txbuffer[5] = 0x00;
    txbuffer[6] = 0x00;
    txbuffer[7] = 0x00;

    /* Copy payload into CAN packet, ensure unused bytes are 0x00. */
    uint8_t i;

    for (i = 0; i < size && i < 20; i++) {
        txbuffer[8+i] = data[i];
    }

    for (; i < 20; i++) {
        txbuffer[8+i] = 0x00;
    }

    /* Request packet is sent, and increment FIFO head. */
    C1TXQCONHbits.UINC = 1;
    C1TXQCONHbits.TXREQ = 1;

    stats.tx_packets++;
}

static const uint8_t dlc_to_bytes[] = {0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 12U, 16U, 20U, 24U, 32U, 48U, 64U};

/**
 * Service the CAN bus, such as receive messages.
 */
void can_service(void) {
    /* Check to see if the RX buffer is too full and has overflowed. */
    if (C1FIFOSTA1Lbits.RXOVIF == 1) {
        C1FIFOSTA1Lbits.RXOVIF = 0;
        stats.rx_overflow++;
    }

    /* Loop through the FIFO for new packets. */
    while (C1FIFOSTA1Lbits.TFNRFNIF == 1) {
        /* Process packet. */
        uint8_t *rxbuffer = (uint8_t *) C1FIFOUA1;

        /* Decode data. */
        uint8_t id = rxbuffer[0];
        uint8_t prefix = rxbuffer[1] & 0b00000111;
        uint8_t dlc = rxbuffer[4] & 0x0f;
        uint8_t size = dlc_to_bytes[dlc];

        stats.rx_packets++;

        /* Pass packet to protocol handling. */
        protocol_receive(prefix, id, size, &rxbuffer[8]);

        /* Increment the buffer. */
        C1FIFOCON1Hbits.UINC = 1;
    }

    if (can_address_phase <= CAN_ADDRESS_CHECKS) {
        can_address_service();

        if (can_ready()) {
            if (can_identifier != can_address_eeprom) {
                nvm_write(EEPROM_LOC_CAN_ID, can_identifier);
            }

            module_set_self_can_id(can_get_id());
        }
    }
}

/**
 * Service CAN address selection process.
 */
void can_address_service(void) {
    if (can_address_phase == 0) {
        if (can_address_eeprom != CAN_NO_ADDRESS && can_identifier == CAN_NO_ADDRESS) {
            can_identifier = can_address_eeprom;
        } else {
            can_identifier = CAN_NO_ADDRESS;

            while(can_identifier == CAN_NO_ADDRESS) {
                can_identifier = rng_generate8(&can_address_seed, CAN_ADDRESS_RNG_MASK);
            }
        }

        can_address_clear_tick = tick_value + (rng_generate8(&can_address_seed, CAN_ADDRESS_RNG_MASK) & 0x7f);
        can_address_phase++;
        stats.id_cycles++;
    } else if (can_address_phase == 1) {
        if (can_address_clear_tick <= tick_value) {
            can_address_phase++;

            can_address_clear_tick = tick_value + 64 + (rng_generate8(&can_address_seed, CAN_ADDRESS_RNG_MASK) & 0x7f);
            protocol_network_address_announce_send();
        }
    } else {
        if (can_address_clear_tick <= tick_value) {
            can_address_phase++;

            if (can_address_phase <= CAN_ADDRESS_CHECKS) {
                can_address_clear_tick = tick_value + 64 + (rng_generate8(&can_address_seed, CAN_ADDRESS_RNG_MASK) & 0x7f);
                protocol_network_address_announce_send();
            }
        }
    }
}

/**
 * Handle receiving notification that our candidate is in use.
 */
void can_address_conflict(uint8_t id) {
    if (id == can_identifier) {
        /* Just reset our progress to selection if we're not decided. */
        if (can_address_phase <= CAN_ADDRESS_CHECKS) {
            can_address_phase = 0;
        }
    }
}

/**
 * Handle an announcement and see if it's in conflict.
 */
void can_address_check(uint8_t id) {
    if (id == can_identifier) {
        protocol_network_address_nak_send();

        can_address_conflict(id);
    }
}

/**
 * Handle updates to the modules domain.
 *
 * @param domain can domain
 */
void can_domain_update(uint8_t domain) {
    if (domain != can_domain) {
        module_set_self_domain(domain);
        can_domain = domain;
        nvm_write(EEPROM_LOC_CAN_DOMAIN, can_domain);
    }
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
 * Get the CAN domain of module.
 *
 * @return the CAN domain
 */
uint8_t can_get_domain(void) {
    return can_domain;
}

/**
 * Return true if the CAN bus is not ready, i.e. has no address id yet.
 *
 * @return true if read.
 */
bool can_ready(void) {
    return can_address_phase > CAN_ADDRESS_CHECKS;
}

/**
 * Return the CAN statistics data.
 *
 * @return CAN statistics
 */
can_statistics_t *can_get_statistics(void) {
    return &stats;
}