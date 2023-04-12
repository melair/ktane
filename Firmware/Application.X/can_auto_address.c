#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include "serial.h"
#include "tick.h"
#include "rng.h"
#include "../common/can.h"
#include "../common/can_private.h"
#include "../common/nvm.h"
#include "../common/eeprom_addrs.h"
#include "../common/packet.h"

#define CAN_ADDRESS_RNG_MASK 0x74926411
#define CAN_ADDRESS_CHECKS 4
#define CAN_NO_ADDRESS 0xff

/* CAN address seed. */
uint32_t can_address_seed;
uint8_t can_address_phase;
uint32_t can_address_clear_tick;
uint8_t can_address_eeprom;

void can_address_initialise(uint8_t initial_id) {
    /* Set our address to no address. */
    can_identifier = CAN_NO_ADDRESS;

    /* Retrieve the CAN EEPROM address. */
    can_address_eeprom = initial_id;

    /* Sanity, if set to 0x00 assume no address. */
    if (can_address_eeprom == 0x00) {
        can_address_eeprom = CAN_NO_ADDRESS;
    }

    /* Set our address seed, our serial number. This should reduce/eliminate
     * conflicts. */
    can_address_seed = serial_get();
    can_address_phase = 0;
}

/**
 * Service CAN address selection process.
 */
void can_address_service(void) {
    if (can_address_phase <= CAN_ADDRESS_CHECKS) {
        if (can_address_phase == 0) {
            if (can_address_eeprom != CAN_NO_ADDRESS && can_identifier == CAN_NO_ADDRESS) {
                can_identifier = can_address_eeprom;
            } else {
                can_identifier = CAN_NO_ADDRESS;

                while (can_identifier == CAN_NO_ADDRESS) {
                    can_identifier = rng_generate8(&can_address_seed, CAN_ADDRESS_RNG_MASK);
                }
            }

            can_address_clear_tick = tick_value + (rng_generate8(&can_address_seed, CAN_ADDRESS_RNG_MASK) & 0x7f);
            can_address_phase++;
            can_stats.id_cycles++;
        } else if (can_address_phase == 1) {
            if (can_address_clear_tick <= tick_value) {
                can_address_phase++;

                can_address_clear_tick = tick_value + 64 + (rng_generate8(&can_address_seed, CAN_ADDRESS_RNG_MASK) & 0x7f);

                packet_outgoing.network.address_announce.serial = serial_get();
                packet_send(PREFIX_NETWORK, OPCODE_NETWORK_ADDRESS_ANNOUNCE, SIZE_NETWORK_ADDRESS_ANNOUNCE, &packet_outgoing);
            }
        } else {
            if (can_address_clear_tick <= tick_value) {
                can_address_phase++;

                if (can_address_phase <= CAN_ADDRESS_CHECKS) {
                    can_address_clear_tick = tick_value + 64 + (rng_generate8(&can_address_seed, CAN_ADDRESS_RNG_MASK) & 0x7f);

                    packet_outgoing.network.address_announce.serial = serial_get();
                    packet_send(PREFIX_NETWORK, OPCODE_NETWORK_ADDRESS_ANNOUNCE, SIZE_NETWORK_ADDRESS_ANNOUNCE, &packet_outgoing);
                }
            }
        }

        if (can_ready()) {
            if (can_identifier != can_address_eeprom) {
                nvm_eeprom_write(EEPROM_LOC_CAN_ID, can_identifier);
            }

            can_is_dirty = true;
        }
    }
}

/**
 * Handle receiving notification that our candidate is in use.
 */
void can_address_receive_nak(uint8_t id, packet_t *p) {
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
void can_address_receive_announce(uint8_t id, packet_t *p) {
    if (id == can_identifier) {
        packet_outgoing.network.address_nak.serial = serial_get();
        packet_send(PREFIX_NETWORK, OPCODE_NETWORK_ADDRESS_NAK, SIZE_NETWORK_ADDRESS_NAK, &packet_outgoing);

        can_address_receive_nak(id, &packet_outgoing);
    }
}

bool can_address_ready(void) {
    return can_address_phase > CAN_ADDRESS_CHECKS;
}