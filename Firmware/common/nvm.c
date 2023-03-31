/**
 * Low level functions for interacting with EEPROM and PFM memory.
 */
#include <xc.h>
#include <stdint.h>
#include "nvm.h"

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
    /* Clear NVCON1, and set command to READ byte. */
    NVMCON1 = 0;
    NVMCON1bits.CMD = 0;

    /* Load address, EEPROM base is 0x380000. */
    NVMADR = (addr & 0x03ff) | 0x380000;

    /* Execute command, and wait until done. */
    NVMCON0bits.GO = 1;
    while(NVMCON0bits.GO == 1);

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
    /* Load address, EEPROM base is 0x380000. */
    NVMADR = (addr & 0x03ff) | 0x380000;

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

    /* Start operation. */
    __asm(" BSF     NVMCON0, 0");

    /* Wait until done, this blocks for update period. */
    while(NVMCON0bits.GO == 1);
}

/**
 * Erase a page of the PFM, 256 bytes - they will be set to 0xff, as writing
 * to the PFM can only make bits go low.
 * 
 * @param addr base of address to wipe
 */
void nvm_pfm_erase(uint24_t addr) {
    /* Clear NVCON1, and set command to ERASE page. */
    NVMCON1 = 0;
    NVMCON1bits.CMD = 6;

    /* Load address of destination for firmware. */
    NVMADR = addr;

    /* Perform unlock procedure. */
    __asm(" MOVLW   0x55");
    __asm(" MOVWF   NVMLOCK");
    __asm(" MOVLW   0xAA");
    __asm(" MOVWF   NVMLOCK");

    /* Start operation. */
    __asm(" BSF     NVMCON0, 0");
    
    while(NVMCON0bits.GO == 1);
}

/**
 * Read word (16bits) from PFM.
 *
 * WARNING: Function contains a loop dependent on SFR bit, though reading
 * from EEPROM should not spin.
 *
 * @param addr PFM address
 * @return word read from program memory
 */
uint16_t nvm_pfm_read(uint24_t addr) {
    /* Clear NVCON1, and set command to READ byte. */
    NVMCON1 = 0;
    NVMCON1bits.CMD = 0;

    /* Load address, EEPROM base is 0x380000. */
    NVMADR = addr;

    /* Execute command, and wait until done. */
    NVMCON0bits.GO = 1;
    while(NVMCON0bits.GO == 1);

    /* Return read byte. */
    return NVMDAT;
}

/**
 * Write word (16bits) to PFM.
 *
 * WARNING: Function contains a loop dependent on SFR bit, writing to the
 * EEPROM will block for a period of time. This could be rewritten to use
 * the interrupt flag NVMIF.
 *
 * @param addr PFM address
 * @param data word to write to program memory
 */
void nvm_pfm_write(uint24_t addr, uint16_t data) {
    /* Load address. */
    NVMADR = addr;

    /* Load byte to be written. */
    NVMDAT = data;

    /* Clear NVCON0, NVCON1, and set command to WRITE byte. */
    NVMCON0 = 0x00;
    NVMCON1 = 0x03;

    /* Perform unlock procedure. */
    __asm(" MOVLW   0x55");
    __asm(" MOVWF   NVMLOCK");
    __asm(" MOVLW   0xAA");
    __asm(" MOVWF   NVMLOCK");

    /* Start operation. */
    __asm(" BSF     NVMCON0, 0");

    /* Wait until done, this blocks for update period. */
    while(NVMCON0bits.GO == 1);
}
