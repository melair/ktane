#include <xc.h>
#include <stdbool.h>
#include "firmware.h"

/* Location of actual firmware. */
#define FIRMWARE_BASE       0x000000 // (0 - 63kb)
#define FIRMWARE_NEW_BASE   0x00FC00 // (63 - 126kb)
#define FIRMWARE_SIZE       0xFC00   // (63kb)

/* Location of flashing function. */
#define FLASH_BASE          0x01F800 // (126kb-128kb)

/* Ensure that the firmware_flash function is linked, even if not called. */
asm("GLOBAL _firmware_flash");

/**
 * Function to flash firmware from FIRMWARE_NEW_BASE to FIRMWARE_BASE.
 * 
 * This function is located at FLASH_BASE, this is done by custom linker
 * options.
 * 
 * Goto "Project Properties -> XC8 Global Options -> XC8 Linker -> Memory model":
 *
 * ROM Ranges:         default,-01f800-01fffe
 * Additional options: -Wl,-Pflasher=01f800h
 * 
 * This reserves the last 1kb of ROM, and sets the "flasher" section location
 * to be 0x01f800.
 * 
 * This function can only call other functions in the "flasher" section, it
 * must not use any global variables. Function must reset MCU upon exit.
 */
void __section("flasher") firmware_flash(void) {
    /* Disable all interrupts. */
    INTCON0bits.IPEN = 0;
    INTCON0bits.GIEH = 0;
    INTCON0bits.GIEL = 0;
    
    /* Change system arbiter to prioritise CPU over all else, should prevent
     * any kind of preemption. */
    PRLOCK = 0x55;
    PRLOCK = 0xAA;
    PRLOCKbits.PRLOCKED = 0;
    
    MAINPR = 0;
    DMA1PR = 1;
    DMA2PR = 2;
    ISRPR = 3;
    SCANPR = 4;    
    
    PRLOCK = 0x55;
    PRLOCK = 0xAA;
    PRLOCKbits.PRLOCKED = 1;
    
    /* Temporary storage for PFM word. */
    uint8_t high = 0x00;
    uint8_t low = 0x00;
    
    /* Loop through firmware size until complete. */
    for (uint24_t addr = 0; addr < FIRMWARE_SIZE; addr += 2) {
        /* Clear watchdog. */
        CLRWDT();
        
        /* For every new page (128 words), perform wipe. */
        if (addr % 256 == 0) {
            /* Clear NVCON1, and set command to ERASE page. */
            NVMCON1 = 0;
            NVMCON1bits.CMD = 6;

            /* Load address of destination for firmware. */
            NVMADR = addr;

            /* Perform unlock procedure. */
            NVMLOCK = 0x55;
            NVMLOCK = 0xaa;

            /* Execute command, and wait until done. */
            NVMCON0bits.GO = 1;
            while(NVMCON0bits.GO);
        }
        
        /* Clear NVCON1, and set command to READ word. */
        NVMCON1 = 0;
        NVMCON1bits.CMD = 0;
        
        /* Load address of source firmware. */
        NVMADR = (FIRMWARE_NEW_BASE + addr);
        
        /* Execute command, and wait until done. */
        NVMCON0bits.GO = 1;
        while(NVMCON0bits.GO);
        
        /* Store PFM. */
        high = NVMDATH;
        low = NVMDATL;
        
        /* Clear NVCON1, and set command to WRITE word. */
        NVMCON1 = 0;
        NVMCON1bits.CMD = 3;
        
        /* Load address of destination for firmware. */
        NVMADR = addr;
        
        /* Load word into NVMDAT. */
        NVMDATH = high;
        NVMDATL = low;
        
        /* Perform unlock procedure. */
        NVMLOCK = 0x55;
        NVMLOCK = 0xaa;

        /* Execute command, and wait until done. */
        NVMCON0bits.GO = 1;
        while(NVMCON0bits.GO);       
    }
 
    /* Restart the microcontroller, hopefully into new firmware. */
    RESET();
}