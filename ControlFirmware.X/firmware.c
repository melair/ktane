#include <xc.h>
#include <stdbool.h>
#include "firmware.h"
#include "modules.h"
#include "protocol_firmware.h"

/* Location of actual firmware. */
#define FIRMWARE_BASE       0x000000 // (0 - 63kb)
#define FIRMWARE_NEW_BASE   0x00FC00 // (63 - 126kb)
#define FIRMWARE_SIZE       0xFC00   // (63kb)

/* Location of flashing function. */
#define FLASH_BASE          0x01F800 // (126kb-128kb)

/* Ensure that the firmware_flash function is linked, even if not called. */
asm("GLOBAL _firmware_flash");

/* Current firmware version. */
const uint16_t firmware_version = 0x000b;
/* CRC32 checksum of firmware, first 63kb of program memory. */
uint32_t firmware_checksum;

/* Firmware state. */
#define FIRMWARE_PROCESS_IDLE       0
#define FIRMWARE_PROCESS_HEADER     1
#define FIRMWARE_PROCESS_PAGES      2
#define FIRMWARE_PROCESS_FAILED     3

uint8_t firmware_state = FIRMWARE_PROCESS_IDLE;

/* New firmware version being downloaded. */
uint16_t firmware_new_version = 0;
/* New firmware checksum. */
uint32_t firmware_new_checksum = 0xffffffff;
/* CAN id of source. */
uint8_t firmware_source_id = 0;
/* Current waiting for page. */
uint16_t firmware_current_page = 0;
/* Total number of pages to fetch. */
uint16_t firmware_total_pages = 0;

/* Local function prototypes. */
uint32_t firmware_calculate_checksum(uint32_t base_addr, uint16_t size);

/**
 * Initialise firmware subsystem, this involves calculating a CRC of the
 * firmware.
 */
void firmware_initialise(void) {
    firmware_checksum = firmware_calculate_checksum(FIRMWARE_BASE, FIRMWARE_SIZE);
}

/**
 * Get the current firmware version.
 * 
 * @return the firmware version
 */
uint16_t firmware_get_version(void) {
    return firmware_version;
}

/**
 * Get the number of pages in new firmware.
 * 
 * @return the number of firmware pages (16 bytes)
 */
uint16_t firmware_get_pages(void) {
    return FIRMWARE_SIZE / 16;
}

/**
 * Get the checksum of the new firmware.
 * 
 * @return the firmware checksum
 */
uint32_t firmware_get_checksum(void) {
    return firmware_checksum;
}

/**
 * Get a page of firmware and place in data.
 * 
 * @param page page to fetch
 * @param data where to store
 */
void firmware_get_page(uint16_t page, uint8_t *data) {
    uint32_t base_addr = page * 16;
        
    for (uint32_t addr = 0; addr < 16; addr += 2) {
        /* Clear NVCON1, and set command to READ word. */
        NVMCON1 = 0;
        NVMCON1bits.CMD = 0;
        
        /* Load address of source firmware. */
        NVMADR = (base_addr + addr);
        
        /* Execute command, and wait until done. */
        NVMCON0bits.GO = 1;
        while(NVMCON0bits.GO == 1);
        
        /* Store page data in array. */
        data[addr] = NVMDATH;
        data[addr + 1] = NVMDATL;
    }
}

/**
 * Check to see if an announced version of firmware is more recent than our
 * current version.
 * 
 * @param new version of firmware
 */
void firmware_check(uint16_t adv_version) {
    /* Ignore firmware checks if we are already in the process of a firmware
     * update. */
    if (firmware_state != FIRMWARE_PROCESS_IDLE) {
        return;       
    }
    
    /* If the advertised version is later than ours, start the process and
     * request a firmware header. */
    if (adv_version > firmware_version) {
        firmware_new_version = adv_version;
        firmware_state = FIRMWARE_PROCESS_HEADER;     
        protocol_firmware_request_send(adv_version);
        
        /* Raise an error to alert network this module is firmwaring. Thus locking
         * the start of a game. */
        module_error_raise(MODULE_ERROR_FIRMWARE_START);
    }
}

/**
 * Receive a firmware header, if this matches the version we are expecting
 * progress to the next stage.
 * 
 * @param version firmware version in header
 * @param pages total number of pages to receive
 * @param crc expected CRC of firmware
 */
void firmware_header_received(uint8_t id, uint16_t version, uint16_t pages, uint32_t crc) {
    if (firmware_state != FIRMWARE_PROCESS_HEADER || version != firmware_new_version) {
        return;       
    }
    
    firmware_source_id = id;
    firmware_total_pages = pages;
    firmware_current_page = 0;
    firmware_new_checksum = crc;   
    firmware_state = FIRMWARE_PROCESS_PAGES;
    
    protocol_firmware_page_request_send(firmware_current_page, firmware_source_id);
}

/**
 * Receive a page of firmware, if the module is currently firmwaring, the source
 * CAN id is expected and the page number is where we're up to, store it in 
 * program flash.
 * 
 * @param id source CAN id
 * @param page page number
 * @param data 16 byte page
 */
void firmware_page_received(uint8_t id, uint16_t page, uint8_t *data) {
    /* Page not for us if we're not performing a firmware, it's not from out
     * source or it's not the current page we want. */
    if (firmware_state != FIRMWARE_PROCESS_PAGES || firmware_source_id != id || firmware_current_page != page) {
        return;       
    }
    
    /* Calculate base address for new page, in second partition. */
    uint32_t page_offset = firmware_current_page << 4;
    uint32_t base_addr = FIRMWARE_NEW_BASE + page_offset;
    
    /* For every new page (128 words), perform wipe. */
    if ((base_addr % 256) == 0) {
        /* Clear Watchdog. */
        CLRWDT();
        
        /* Clear NVCON1, and set command to ERASE page. */
        NVMCON1 = 0;
        NVMCON1bits.CMD = 6;

        /* Load address of destination for firmware. */
        NVMADR = base_addr;

        /* Perform unlock procedure. */
        NVMLOCK = 0x55;
        NVMLOCK = 0xaa;

        /* Execute command, and wait until done. */
        NVMCON0bits.GO = 1;
        while(NVMCON0bits.GO == 1);
    }
    
    /* Loop through payload and store in PFM. */
    for (uint32_t addr = 0; addr < 16; addr += 2) {
        /* Clear Watchdog. */
        CLRWDT();
        
         /* Clear NVCON1, and set command to WRITE word. */
        NVMCON1 = 0;
        NVMCON1bits.CMD = 3;
        
        /* Load address of destination for firmware. */
        NVMADR = base_addr + addr;
        
        /* Load word into NVMDAT. */
        NVMDATH = data[addr];
        NVMDATL = data[addr+1];
        
        /* Perform unlock procedure. */
        NVMLOCK = 0x55;
        NVMLOCK = 0xaa;

        /* Execute command, and wait until done. */
        NVMCON0bits.GO = 1;
        while(NVMCON0bits.GO == 1);       
    }
    
    
    firmware_current_page++;    
    /* If the last page has been fetched. */
    if (firmware_current_page >= firmware_total_pages) {
        /* Calculate the new CRC. */
        uint32_t crc = firmware_calculate_checksum(FIRMWARE_NEW_BASE, FIRMWARE_SIZE);
        
        /* If the sent checksum matches the calculated checksum, begin the final stage. */
        if (crc == firmware_new_checksum) {
            /* This function copies from FIRMWARE_NEW_BASE to FIRMWARE_BASE, it will
             * not return, it will RESET() the microcontroller upon completion. */
            firmware_flash();
        }
        
        /* If we hit this, something has failed. */
        firmware_state = FIRMWARE_PROCESS_FAILED;       
        
        /* Raise an error to alert network this module has failed to firmware. */
        module_error_raise(MODULE_ERROR_FIRMWARE_FAILED);
    } else {
        protocol_firmware_page_request_send(firmware_current_page, firmware_source_id);   
    }
}

/**
 * Checksum the existing firmware, CRC32 using hardware.
 * 
 * @return 32bit checksum
 */
uint32_t firmware_calculate_checksum(uint32_t base_addr, uint16_t size) {
    /* Set polynomial length (-1). */
    CRCCON1bits.PLEN = 31;
    
    /* Set data length (-1). */
    CRCCON2bits.DLEN = 15;
    
    /* Set standard 32-bit polynomial. */
    CRCCON0bits.SETUP = 0b10;
    CRCXORT = 0x04;
    CRCXORU = 0xc1;
    CRCXORH = 0x1d;
    CRCXORL = 0xb7;
    
    /* Initialise output data. */
    CRCCON0bits.SETUP = 0b00;
    CRCOUTT = 0xff;
    CRCOUTU = 0xff;
    CRCOUTH = 0xff;
    CRCOUTL = 0xff;
    
    /* Clear data. */
    CRCDATAT = 0x00;
    CRCDATAU = 0x00;
    CRCDATAH = 0x00;
    CRCDATAL = 0x00;
    
    /* Set CRC to Accumulator mode.*/
    CRCCON0bits.ACCM = 1;
    
    /* Enable CRC module. */
    CRCCON0bits.EN = 1;
    
    /* Start CRC module. */
    CRCCON0bits.GO = 1;
    
    for (uint32_t addr = 0; addr < size; addr += 2) {
        /* Clear watchdog. */
        CLRWDT();
        
        /* Clear NVCON1, and set command to READ word. */
        NVMCON1 = 0;
        NVMCON1bits.CMD = 0;
        
        /* Load address of source firmware. */
        NVMADR = base_addr + ((uint32_t) addr);
        
        /* Execute command, and wait until done. */
        NVMCON0bits.GO = 1;
        while(NVMCON0bits.GO == 1);
        
        /* Loop while buffer full, though shouldn't be. */
        while(CRCCON0bits.FULL == 1);
        
        /* Pass to CRC. */       
        CRCDATAH = NVMDATH;
        CRCDATAL = NVMDATL;

        /* Wait while CRC is running. */
        while(CRCCON0bits.BUSY == 1);
    }
    
    /* Stop CRC module. */
    CRCCON0bits.GO = 0;
    
    /* Disable CRC module. */
    CRCCON0bits.EN = 0;
    
    return CRCOUT;
}

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
    for (uint32_t addr = 0; addr < FIRMWARE_SIZE; addr += 2) {
        /* Clear watchdog. */
        CLRWDT();
        
        /* For every new page (128 words), perform wipe. */
        if ((addr % 256) == 0) {
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
            while(NVMCON0bits.GO == 1);
        }
        
        /* Clear NVCON1, and set command to READ word. */
        NVMCON1 = 0;
        NVMCON1bits.CMD = 0;
        
        /* Load address of source firmware. */
        NVMADR = (FIRMWARE_NEW_BASE + addr);
        
        /* Execute command, and wait until done. */
        NVMCON0bits.GO = 1;
        while(NVMCON0bits.GO == 1);
        
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
        while(NVMCON0bits.GO == 1);       
    }
 
    /* Restart the microcontroller, hopefully into new firmware. */
    RESET();
}
