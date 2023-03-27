#include <xc.h>
#include <stdbool.h>

#define PROGRAM_VECTOR 0x000100
#define FLASHER_VECTOR 0x01f000

#define _str(x) #x
#define str(x)  _str(x)

bool boot_to_program(void) {
    /* Clear NVCON1, and set command to READ byte. */
    NVMCON1 = 0;
    NVMCON1bits.CMD = 0;

    /* Read last byte of EEPROM. */
    NVMADR = 0x3803ff;

    /* Execute command, and wait until done. */
    NVMCON0bits.GO = 1;
    while(NVMCON0bits.GO == 1);

    /* Check if we should boot to program. */
    return (NVMDATL & 0b00000001);
}

void main(void) {
    /* Store if we need to boot to program. */
    bool program = boot_to_program();
          
    /* Reset stack pointer. */
    STKPTR = 0x00;
    
    /* Reset interrupt return. */
	BSR = 0x00;
    
    /* Jump to program. */
    if (program) {
        asm ("goto  " str(PROGRAM_VECTOR));
    } else {
        asm ("goto  " str(FLASHER_VECTOR));
    }
}

