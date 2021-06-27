#ifndef NVM_H
#define	NVM_H

#include <stdint.h>

void nvm_initialise(void);
uint8_t nvm_read(uint16_t addr);
void nvm_write(uint16_t addr, uint8_t data);

/* PIC18F57Q84 EEPROM, 0x000-0x3ff. */

#define EEPROM_LOC_VERSION_HIGH                     0x000 /* Version of EEPROM data, allowing setting of defaults. */
#define EEPROM_LOC_VERSION_LOW                      0x001 

#define EEPROM_LOC_MODE_CONFIGURATION               0x010 /* Module configuration mode. */
#define EEPROM_LOC_CAN_ID                           0x011 /* Configured CAN identifier. */

#define EEPROM_LOC_LCD_BRIGHTNESS                   0x020 /* LCD Backlight Brightness. */
#define EEPROM_LOC_LCD_CONTRAST                     0x021 /* LCD Contrast. */

#endif	/* NVM_H */
