#ifndef NVM_ADDRS_H
#define	NVM_ADDRS_H

/* Application (0x000-0x3df) */
#define EEPROM_LOC_VERSION_HIGH                     0x000 /* Version of EEPROM data, allowing setting of defaults. */
#define EEPROM_LOC_VERSION_LOW                      0x001

#define EEPROM_LOC_MODE_CONFIGURATION               0x010 /* Module configuration mode. */
#define EEPROM_LOC_CAN_ID                           0x011 /* Configured CAN identifier. */

#define EEPROM_LOC_RNG_A                            0x012 /* RNG Seed, A */
#define EEPROM_LOC_RNG_B                            0x013 /* RNG Seed, B */
#define EEPROM_LOC_RNG_C                            0x014 /* RNG Seed, C */
#define EEPROM_LOC_RNG_D                            0x015 /* RNG Seed, D */

#define EEPROM_LOC_OPT_KPORTA                       0x018 /* Optional module on KPORTA. */
#define EEPROM_LOC_OPT_KPORTB                       0x019 /* Optional module on KPORTB. */
#define EEPROM_LOC_OPT_KPORTC                       0x01a /* Optional module on KPORTC. */

#define EEPROM_LOC_LCD_BRIGHTNESS                   0x020 /* LCD Backlight Brightness. */
#define EEPROM_LOC_LCD_CONTRAST                     0x021 /* LCD Contrast. */

#define EEPROM_LOC_ARGB_BRIGHTNESS                  0x022 /* ARGB Global Brightness. */
#define EEPROM_LOC_BUZZER_VOL                       0x023 /* Buzzer Volume. */

/* Bootloader (0x3e0-0x3ef) */
#define EEPROM_LOC_BOOTLOADER_TARGET                0x3e0 /* Bootloader Target (0xFF = Application, 0x00 = Flasher). */

/* Flasher (0x3f0-0x3ff) */
#define EEPROM_LOC_FLASHER_SEGMENT                  0x3f0 /* Firmware segment to update. */
#define EEPROM_LOC_FLASHER_VERSION_HIGHER           0x3f1 /* Firmware version to update. */
#define EEPROM_LOC_FLASHER_VERSION_LOWER            0x3f2 /* Firmware version to update. */

#endif	/* NVM_ADDRS_H */

