#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include "../common/mcu.h"
#include "../common/device.h"
#include "../common/fw.h"
#include "../common/fw_updater.h"
#include "../common/can.h"
#include "../common/nvm.h"
#include "../common/eeprom_addrs.h"

/* Flasher Version, little endian. */
asm("PSECT flasherversion");
asm("dw 0x0001");

void main(void) {
    /* Drive everything unused to low. */
    safe_unused_pins();

    /* Reconfigure system arbiter. */
    arbiter_initialise();

    /* Configure sleep behaviour, put CPU in IDLE. */
    CPUDOZEbits.IDLEN = 1;

    /* Load versions from PFM. */
    fw_initialise();

    /* Unlock PPS during initialisation. */
    pps_unlock();

    /* Initialise CAN bus. */
    can_initialise();

    /* Lock PPS for safety during flashing. */
    pps_lock();

    /* Load the segment and firmware version to attempt from EEPROM. */
    uint8_t segment = nvm_eeprom_read(EEPROM_LOC_FLASHER_SEGMENT);
    uint16_t version = ((uint16_t) nvm_eeprom_read(EEPROM_LOC_FLASHER_VERSION_HIGHER) << 8) + ((uint16_t) nvm_eeprom_read(EEPROM_LOC_FLASHER_VERSION_LOWER));

    /* Set firmware updater in action. */
    fw_updater_start(segment, version);

    while (true) {
        /* Clear watchdog. */
        CLRWDT();

        /* Service CAN buffers. */
        can_service();

        /* Put MCU into IDLE mode, no CPU instructions but peripherals continue.
         *
         * Full SLEEP would be ideal, but is incompatible with the CAN peripheral
         * as SLEEP disables the memory controller. CAN can wake upon start of
         * message but the packet would likely be lost. This behaviour would be
         * suitable for a device that comes on once every few seconds, but for
         * something that needs to wake up at least every 200ms, it wont work.
         *
         * CPU will wake from the tick timer. This doesn't save much power, but
         * it prevents functions spinning that don't need to operate faster than
         * the tick refresh. */
        //  SLEEP();
    };
}

