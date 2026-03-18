#ifndef NVM_H
#define	NVM_H

#include <stdint.h>

uint8_t nvm_eeprom_read(uint16_t addr);
void nvm_eeprom_write(uint16_t addr, uint8_t data);

#endif