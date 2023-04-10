#include <xc.h>
#include <stdbool.h>
#include <stdint.h>
#include "../common/eeprom_addrs.h"
#include "../common/segments.h"
#include "../common/device.h"
#include "../common/nvm.h"

#define _str(x) #x
#define str(x)  _str(x)

/* Bootloader firmware version, little endian. */
asm("PSECT bootloaderversion");
asm("dw 0x0001");

void main(void) {
    /* Read destination and reset back to application. */
    uint8_t destination = nvm_eeprom_read(EEPROM_LOC_BOOTLOADER_TARGET);
    nvm_eeprom_write(EEPROM_LOC_BOOTLOADER_TARGET, 0xff);

    /* Reset stack pointer. */
    STKPTR = 0x00;

    /* Reset interrupt return. */
    BSR = 0x00;

    /* Jump to program. */
    if (destination == 0x00) {
        asm("goto  " str(FLASHER_OFFSET));
    } else {
        asm("goto  " str(APPLICATION_OFFSET));
    }
}
