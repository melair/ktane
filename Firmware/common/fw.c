#include <xc.h>
#include <stdint.h>
#include "nvm.h"
#include "segments.h"
#include "fw.h"

/* Cached values. */
uint16_t fw_versions[SEGMENT_COUNT];
uint32_t fw_checksums[SEGMENT_COUNT];

/* Locations in PFM memory for versions. */
const uint32_t fw_version_locations[SEGMENT_COUNT] = {
    BOOTLOADER_OFFSET + BOOTLOADER_SIZE - 2,
    APPLICATION_OFFSET + APPLICATION_SIZE - 2,
    FLASHER_OFFSET + FLASHER_SIZE - 2
};

const uint32_t fw_offsets[SEGMENT_COUNT] = {
    BOOTLOADER_OFFSET,
    APPLICATION_OFFSET,
    FLASHER_OFFSET
};

const uint32_t fw_sizes[SEGMENT_COUNT] = {
    BOOTLOADER_SIZE,
    APPLICATION_SIZE,
    FLASHER_SIZE
};

#define FW_PAGE_SIZE 16

const uint16_t fw_pages[SEGMENT_COUNT] = {
    BOOTLOADER_SIZE / FW_PAGE_SIZE,
    APPLICATION_SIZE / FW_PAGE_SIZE,
    FLASHER_SIZE / FW_PAGE_SIZE
};

void fw_initialise(void) {
    for (uint8_t i = 0; i < SEGMENT_COUNT; i++) {
        fw_versions[i] = nvm_pfm_read(fw_version_locations[i]);
        fw_checksums[i] = fw_calculate_checksum(fw_offsets[i], fw_sizes[i]);
    }
}

uint16_t fw_version(uint8_t segment) {
    return fw_versions[segment];
}

uint32_t fw_checksum(uint8_t segment) {
    return fw_checksums[segment];
}

uint16_t fw_page_count(uint8_t segment) {
    return fw_pages[segment];
}

void fw_page_read(uint8_t segment, uint16_t page, uint8_t *data) {
    uint32_t base_addr = fw_offsets[segment] + (page * FW_PAGE_SIZE);
    uint8_t off = 0;

    for (uint32_t addr = base_addr; addr < base_addr + FW_PAGE_SIZE; addr += 2) {
        uint16_t pfm = nvm_pfm_read(addr);

        /* Store page data in array. */
        data[off++] = (pfm >> 8) & 0xff;
        data[off++] = pfm & 0xff;
    }
}

void fw_page_write(uint8_t segment, uint16_t page, uint8_t *data) {
    uint32_t base_addr = fw_offsets[segment] + (page * FW_PAGE_SIZE);

    /* For every new page (128 words), perform wipe. */
    if ((base_addr & 0xff) == 0) {
        nvm_pfm_erase(base_addr);
    }

    uint8_t off = 0;

    /* Loop through payload and store in PFM. */
    for (uint32_t addr = base_addr; addr < base_addr + FW_PAGE_SIZE; addr += 2) {
        uint16_t out = (uint16_t) (data[off] << 8) | data[off + 1];
        nvm_pfm_write(addr, out);
        off += 2;
    }
}

uint32_t fw_calculate_checksum(uint32_t base_addr, uint32_t size) {
    /* Set polynomial length (-1). */
    CRCCON1bits.PLEN = 31;

    /* Set data length (-1). */
    CRCCON2bits.DLEN = 15;

    /* Set standard 32-bit polynomial. */
    CRCCON0bits.SETUP = 0x02;
    CRCXORT = 0x04;
    CRCXORU = 0xc1;
    CRCXORH = 0x1d;
    CRCXORL = 0xb7;

    /* Initialise output data. */
    CRCCON0bits.SETUP = 0x00;
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

    for (uint32_t addr = base_addr; addr < (base_addr + size); addr += 2) {
        /* Clear watchdog. */
        CLRWDT();

        /* Clear NVCON1, and set command to READ word. */
        NVMCON1 = 0;
        NVMCON1bits.CMD = 0;

        /* Load address of source firmware. */
        NVMADRL = addr & 0xff;
        NVMADRH = (addr >> 8) & 0xff;
        NVMADRU = (addr >> 16) & 0xff;

        /* Execute command, and wait until done. */
        NVMCON0bits.GO = 1;
        while (NVMCON0bits.GO == 1);

        /* Loop while buffer full, though shouldn't be. */
        while (CRCCON0bits.FULL == 1);

        /* Pass to CRC. */
        CRCDATAH = NVMDATH;
        CRCDATAL = NVMDATL;

        /* Wait while CRC is running. */
        while (CRCCON0bits.BUSY == 1);
    }

    /* Stop CRC module. */
    CRCCON0bits.GO = 0;

    /* Disable CRC module. */
    CRCCON0bits.EN = 0;

    return CRCOUT;
}
