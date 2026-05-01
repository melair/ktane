/**
 * Low level functions for interacting with EEPROM and PFM memory.
 */
#include "nvm.h"
#include <stdint.h>
#include <xc.h>
#include "polyfill/pic.h"

/**
 * Read byte from NVM EEPROM.
 *
 * WARNING: Function contains a loop dependent on SFR bit, though reading
 * from EEPROM should not spin.
 *
 * @param addr EEPROM relative address 0x000-0x3ff
 * @param ptr destination of read data
 * @param len length of data to read from EEPROM
 */
void nvm_eeprom_read(uint16_t addr, void *ptr, uint8_t len) {
  uint8_t *out = (uint8_t *)ptr;

  for (uint8_t i = 0; i < len; i++) {
    /* Clear NVCON0, NVCON1, setting command to READ byte. */
    NVMCON0 = 0x00;
    NVMCON1 = 0x00;

    /* Load address, EEPROM base is 0x380000. */
    NVMADRL = addr & 0xff;
    NVMADRH = (addr >> 8) & 0x03;
    NVMADRU = 0x38;

    /* Execute command, and wait until done. */
    NVMCON0bits.GO = 1;
    while (NVMCON0bits.GO == 1)
      ;

    *out = NVMDATL;
  }
}

/**
 * Write byte to NVM EEPROM.
 *
 * WARNING: Function contains a loop dependent on SFR bit, writing to the
 * EEPROM will block for a period of time. This could be rewritten to use
 * the interrupt flag NVMIF.
 *
 * @param addr EEPROM relative address 0x000-0x3ff
 * @param ptr destination of write data
 * @param len length of data to write to EEPROM
 */
void nvm_eeprom_write(uint16_t addr, void *ptr, uint8_t len) {
  uint8_t *in = (uint8_t *)ptr;

  for (uint8_t i = 0; i < len; i++) {
    /* Load address, EEPROM base is 0x380000. */
    NVMADRL = addr & 0xff;
    NVMADRH = (addr >> 8) & 0x03;
    NVMADRU = 0x38;

    /* Load byte to be written. */
    NVMDATL = *in;

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
    while (NVMCON0bits.GO == 1)
      ;
  }
}