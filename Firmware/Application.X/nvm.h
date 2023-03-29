#ifndef NVM_H
#define	NVM_H

#include <stdint.h>

void nvm_initialise(void);
uint8_t nvm_read(uint16_t addr);
void nvm_write(uint16_t addr, uint8_t data);

uint16_t nvm_read_pfm(uint24_t addr);
void nvm_write_pfm(uint24_t addr, uint16_t data);

#endif	/* NVM_H */
