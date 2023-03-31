#include <xc.h>
#include <stdint.h>
#include "nvm.h"
#include "segments.h"
#include "versions.h"

/* Cached versions. */
uint16_t versions[3];

/* Locations in PFM memory for versions. */
const uint32_t version_locations[3] = { BOOTLOADER_OFFSET + BOOTLOADER_SIZE - 2, APPLICATION_OFFSET + APPLICATION_SIZE - 2, FLASHER_OFFSET + FLASHER_SIZE - 2 };

void versions_initialise(void) {
    for (uint8_t i = 0; i < 3; i++) {
        versions[i] = nvm_pfm_read(version_locations[i]);
    }
}

uint16_t versions_get(uint8_t v) {
    return versions[v];
}
