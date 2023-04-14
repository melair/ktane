/**
 * Provide NVM EEPROM functionality so that other functions can easily store
 * persistent data.
 */
#include <xc.h>
#include <stdint.h>
#include "../common/nvm.h"
#include "../common/eeprom_addrs.h"

/* Current version of EEPROM data, bump when new EEPROM parameters are added. */
#define EEPROM_CURRENT_VERSION 5

/**
 * Initialise any EEPROM data required, used to migrate EEPROM data versions
 * from one to another.
 */
void eeprom_initialise(void) {
    uint16_t read_version = 0;

    /* Read the current EEPROM version, stored as uint16. */
    read_version = (uint16_t) (nvm_eeprom_read(EEPROM_LOC_VERSION_HIGH) << 8);
    read_version |= (uint16_t) (nvm_eeprom_read(EEPROM_LOC_VERSION_LOW));

    /* If version is 0xffff, EEPROM is uninitialised. */
    if (read_version == 0xffff) {
        read_version = 0;
    } else if (read_version > EEPROM_CURRENT_VERSION) {
        read_version = 0;
    }

    /* If the current EEPROM version is too low, then update.
     *
     * Important: Migrations should be idempotent, if data needs to be modified
     * then it should be migrated to another location, and the location #define
     * changed to that. This prevents issues if the migration is interrupted
     * before the new version is written.
     *
     * This is a likely event during initial power up, especially from development
     * tools, or PSU brown outs due to inrush currents. Another defence is to
     * checkpoint the EEPROM version during the migrations, this could mean
     * that the EEPROM version increments greater than 1 per version.
     */
    if (read_version < EEPROM_CURRENT_VERSION) {
        if (read_version < 1) {
            nvm_eeprom_write(EEPROM_LOC_MODE_CONFIGURATION, 0x00);
            nvm_eeprom_write(EEPROM_LOC_CAN_ID, 0x00);
        }

        if (read_version < 2) {
            nvm_eeprom_write(EEPROM_LOC_LCD_BRIGHTNESS, 0xff);
            nvm_eeprom_write(EEPROM_LOC_LCD_CONTRAST, 0x3f);
        }

        if (read_version < 4) {
            nvm_eeprom_write(EEPROM_LOC_OPT_KPORTA, 0x00);
            nvm_eeprom_write(EEPROM_LOC_OPT_KPORTB, 0x00);
            nvm_eeprom_write(EEPROM_LOC_OPT_KPORTC, 0x00);
            nvm_eeprom_write(EEPROM_LOC_OPT_KPORTD, 0x00);
        }

        if (read_version < 5) {
            nvm_eeprom_write(EEPROM_LOC_ARGB_BRIGHTNESS, 31);
            nvm_eeprom_write(EEPROM_LOC_BUZZER_VOL, 7);
        }

        /* Write the current EEPROM version. */
        nvm_eeprom_write(EEPROM_LOC_VERSION_HIGH, (EEPROM_CURRENT_VERSION >> 8) & 0xff);
        nvm_eeprom_write(EEPROM_LOC_VERSION_LOW, EEPROM_CURRENT_VERSION & 0xff);
    }
}
