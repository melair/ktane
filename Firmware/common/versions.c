#include <xc.h>
#include <stdint.h>
#include "nvm.h"
#include "segments.h"
#include "versions.h"

uint16_t versions[3];

void versions_initialise(void) {
    for (uint8_t i = 0; i < 3; i++) {
        uint32_t addr;
        
        switch(i) {
            case BOOTLOADER_VERSION:
                addr = BOOTLOADER_OFFSET + BOOTLOADER_SIZE - 2;
                break;
            case APPLICATION_VERSION:
                addr = APPLICATION_OFFSET + APPLICATION_SIZE - 2;
                break;
            case FLASHER_VERSION:
                addr = FLASHER_OFFSET + FLASHER_SIZE - 2;
                break;
        }
        
        versions[i] = nvm_pfm_read(addr);
    }
}

uint16_t versions_get(uint8_t v) {
    return versions[v];
}
