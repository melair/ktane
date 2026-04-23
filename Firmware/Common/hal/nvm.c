/**
 * Low level functions for interacting with EEPROM and PFM memory.
 */
#include "nvm.h"
#include <stdint.h>
#include <xc.h>

/**
 * Get the EEPROM size.
 */
inline uint16_t nvm_eeprom_size(void) { return _DCI_EESIZ; }

/**
 * Read byte from NVM EEPROM.
 *
 * WARNING: Function contains a loop dependent on SFR bit, though reading
 * from EEPROM should not spin.
 *
 * @param addr EEPROM relative address 0x000-0x3ff
 * @return byte read from eeprom
 */
uint8_t nvm_eeprom_read(uint16_t addr) {
  if (addr >= nvm_eeprom_size()) {
    return 0;
  }

  /* Clear NVCON0, NVCON1, setting command to READ byte. */
  NVMCON0 = 0x00;
  NVMCON1 = 0x00;

  /* Load address, EEPROM base is 0x380000. */
  NVMADRL = addr & 0xff;
  NVMADRH = (addr >> 8) & 0x03;
  NVMADRU = 0x38;

  /* Execute command, and wait until done. */
  NVMCON0bits.GO = 1;
  while (NVMCON0bits.GO == 1);

  /* Return read byte. */
  return NVMDATL;
}

/**
 * Write byte to NVM EEPROM.
 *
 * WARNING: Function contains a loop dependent on SFR bit, writing to the
 * EEPROM will block for a period of time. This could be rewritten to use
 * the interrupt flag NVMIF.
 *
 * @param addr EEPROM relative address 0x000-0x3ff
 * @param data byte to write to EEPROM
 */
void nvm_eeprom_write(uint16_t addr, uint8_t data) {
  if (addr >= nvm_eeprom_size()) {
    return;
  }

  /* Load address, EEPROM base is 0x380000. */
  NVMADRL = addr & 0xff;
  NVMADRH = (addr >> 8) & 0x03;
  NVMADRU = 0x38;

  /* Load byte to be written. */
  NVMDATL = data;

  /* Clear NVCON0, NVCON1, and set command to WRITE byte. */
  NVMCON0 = 0x00;
  NVMCON1 = 0x03;

  /* Perform unlock procedure. */
  __asm(" MOVLW   0x55");
  __asm(" MOVWF   NVMLOCK");
  __asm(" MOVLW   0xAA");
  __asm(" MOVWF   NVMLOCK");

  /* Execute command, and wait until done. */
  NVMCON0bits.GO = 1;
  while (NVMCON0bits.GO == 1);
}