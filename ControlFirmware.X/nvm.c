/**
 * Provide NVM EEPROM functionality so that other functions can easily store
 * persistent data.
 * 
 * EEPROM location reservations are stored in nvm.h.
 */
#include <xc.h>
#include <stdint.h>
#include "nvm.h"

/* Current version of EEPROM data, bump when new EEPROM parameters are added. */
#define EEPROM_CURRENT_VERSION 2

/**
 * Initialise any EEPROM data required, used to migrate EEPROM data versions
 * from one to another.
 */
void nvm_initialise(void) {
    uint16_t read_version = 0;
    
    /* Read the current EEPROM version, stored as uint16. */
    read_version = (uint16_t) (nvm_read(EEPROM_LOC_VERSION_HIGH) << 8);
    read_version |= (uint16_t) (nvm_read(EEPROM_LOC_VERSION_LOW));
    
    /* If version is 0xffff, EEPROM is uninitialised. */
    if (read_version == 0xffff) {
        read_version = 0;
    } else if (read_version > EEPROM_CURRENT_VERSION) {
        read_version = 0;
    }
    
    /* If the current EEPROM version is too low, then update. 
     *
     * Important: Migrations should be idempotent, if data needs to be modified
     * then it should be migrated to another location, and the location #define
     * changed to that. This prevents issues if the migration is interrupted
     * before the new version is written.
     * 
     * This is a likely event during initial power up, especially from development
     * tools, or PSU brown outs due to inrush currents. Another defence is to
     * checkpoint the EEPROM version during the migrations, this could mean
     * that the EEPROM version increments greater than 1 per version.
     */
    if (read_version < EEPROM_CURRENT_VERSION) {              
        /* Write the current EEPROM version. */
        nvm_write(EEPROM_LOC_VERSION_HIGH, (EEPROM_CURRENT_VERSION >> 8) & 0xff);
        nvm_write(EEPROM_LOC_VERSION_LOW, EEPROM_CURRENT_VERSION & 0xff);
        
        if (read_version < 1) {
            nvm_write(EEPROM_LOC_MODE_CONFIGURATION, 0x00);
            nvm_write(EEPROM_LOC_CAN_ID, 0x00);
        }
        
        if (read_version < 2) {
            nvm_write(EEPROM_LOC_LCD_BRIGHTNESS, 0xff);
            nvm_write(EEPROM_LOC_LCD_CONTRAST, 0x3f);
        }
    }
}

/**
 * Read byte from NVM EEPROM.
 * 
 * WARNING: Function contains a loop dependent on SFR bit, though reading
 * from EEPROM should not spin.
 * 
 * @param addr EEPROM relative address 0x000-0x3ff
 * @return byte read from eeprom
 */
uint8_t nvm_read(uint16_t addr) {
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
 * Writes are likely infrequent however:
 *  - Completion of slider calibration.
 *  - New controller bus direction (usually only on start up).
 * 
 * While the EEPROM is unlocked, global interrupts are disabled for two 
 * instructions, though this is unlikely to cause any issue.
 * 
 * @param addr EEPROM relative address 0x000-0x3ff
 * @param data byte to write to EEPROM
 */
void nvm_write(uint16_t addr, uint8_t data) {
    /* Load address, EEPROM base is 0x380000. */
    NVMADR = (addr & 0x03ff) | 0x380000;
           
    /* Load byte to be written. */
    NVMDATL = data;
    
    /* Clear NVCON0, NVCON1, and set command to WRITE byte. */
    NVMCON0 = 0x00;
    NVMCON1 = 0x03;
    
    /* Disable interrupts. */
    INTCON0bits.GIE = 0;    
    
    /* Perform unlock procedure. */
    __asm(" MOVLW   0x55");
    __asm(" MOVWF   NVMLOCK");
    __asm(" MOVLW   0xAA");
    __asm(" MOVWF   NVMLOCK");
    
    /* Start operation. */
    __asm(" BSF     NVMCON0, 0");
         
    /* Wait until done, this blocks for update period. */
    while(NVMCON0bits.GO == 1);
    
    /* Re-enable interrupts. */
    INTCON0bits.GIE = 1;
}


