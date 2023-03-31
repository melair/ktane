#ifndef NVM_H
#define	NVM_H

#include <stdint.h>

uint8_t nvm_eeprom_read(uint16_t addr);
void nvm_eeprom_write(uint16_t addr, uint8_t data);

void nvm_pfm_erase(uint32_t addr);
uint16_t nvm_pfm_read(uint32_t addr);
void nvm_pfm_write(uint32_t addr, uint16_t data);

#endif	/* NVM_H */

