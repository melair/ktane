#ifndef CONFIG_H
#define CONFIG_H

#include <hal/nvm.h>

#define CONFIG_LOC_APPLICATION (NVM_EEPROM_BOOTLOADER_BASE + 0x00)

void config_init(void);

#endif