#include <xc.h>
#include <stdbool.h>
#include <stdint.h>
#include "../common/eeprom_addrs.h"
#include "../common/segments.h"
#include "../common/device.h"

#define _str(x) #x
#define str(x)  _str(x)

asm ("PSECT bootloaderversion");
asm ("dw 0x0001");

bool boot_to_application(void) {
    /* Clear NVCON1, and set command to READ byte. */
    NVMCON1 = 0;
    NVMCON1bits.CMD = 0;

    /* Read last byte of EEPROM. */    
    NVMADRL = EEPROM_LOC_BOOTLOADER_TARGET & 0xff;
    NVMADRH = (EEPROM_LOC_BOOTLOADER_TARGET >> 8) & 0xff;
    NVMADRU = 0x38;

    /* Execute command, and wait until done. */
    NVMCON0bits.GO = 1;
    while(NVMCON0bits.GO == 1);

    /* Check if we should boot to program. */
    return (NVMDATL & 0x01);
}

void main(void) {
    /* Store if we need to boot to program. */
    bool to_app = boot_to_application();
          
    /* Reset stack pointer. */
    STKPTR = 0x00;
    
    /* Reset interrupt return. */
    BSR = 0x00;
    
    /* Jump to program. */
    if (to_app) {
        asm ("goto  " str(APPLICATION_OFFSET));
    } else {
        asm ("goto  " str(FLASHER_OFFSET));
    }
}
