#include <xc.h>
#include <hal/nvm.h>
#include <utils/mem.h>
#include "config.h"

#define CURRENT_VERSION 0x0001

void config_init(void) {
    uint16_t version;
    nvm_eeprom_read(CONFIG_LOC_VERSION, &version, sizeof(uint16_t));

    if (version > CURRENT_VERSION) {
        version = 0;
    }

    uint8_t t;
    switch (version) {
        case 0:
            t = 0x00;
            nvm_eeprom_write(CONFIG_LOC_MODE, &t, sizeof(uint8_t));
    }

    if (version != CURRENT_VERSION) {
        version = CURRENT_VERSION;
        nvm_eeprom_write(CONFIG_LOC_VERSION, &version, sizeof(uint16_t));
    }
}