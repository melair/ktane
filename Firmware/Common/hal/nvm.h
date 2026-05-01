#ifndef NVM_H
#define	NVM_H

#include <stdint.h>
#include <xc.h>
#include "polyfill/pic.h"

void nvm_eeprom_read(uint16_t addr, void *ptr, uint8_t len);
void nvm_eeprom_write(uint16_t addr, void *ptr, uint8_t len);

#define NVM_EEPROM_FLASHER_SIZE 0x10
#define NVM_EEPROM_BOOTLOADER_SIZE 0x10
#define NVM_EEPROM_APPLICATION_SIZE (EEPROM_SIZE - NVM_EEPROM_FLASHER_SIZE - NVM_EEPROM_BOOTLOADER_SIZE)

#define NVM_EEPROM_APPLICATION_BASE 0x000
#define NVM_EEPROM_FLASHER_BASE (NVM_EEPROM_APPLICATION_BASE + NVM_EEPROM_APPLICATION_SIZE)
#define NVM_EEPROM_BOOTLOADER_BASE (NVM_EEPROM_FLASHER_BASE + NVM_EEPROM_FLASHER_SIZE)

#endif