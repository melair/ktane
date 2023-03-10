#include <xc.h>
#include "opts.h"
#include "nvm.h"

void opts_initialise_port(uint8_t port);

#define OPT_ID_MASK     0b11110000
#define OPT_DATA_MASK   0b00001111

#define OPT_NONE        0b00000000
#define OPT_PSU         0b00010000
#define OPT_SPI         0b00100000
#define OPT_AUDIO       0b00110000

const uint8_t OPT_PORTS[5] = { 
    0b00000000, 
    0b00001000, // PSU is valid in KPORTD only, I2C.
    0b00001100, // SPI is valid in KPORTC/D, SPI1 on D, SPI2 on C. 
    0b00000001, // AUDIO is valid in KPORTA only, DAC.
};

void opts_initialise(void) {
    for (uint8_t i = 0; i < 4; i++) {
        opts_initialise_port(i);
    }
}

void opts_initialise_port(uint8_t port) {
    uint8_t opt = nvm_read(EEPROM_LOC_OPT_KPORTA + port);
    
    uint8_t id = opt & OPT_ID_MASK;
    uint8_t data = opt & OPT_DATA_MASK;
    
    if (id == OPT_NONE) {
        return;
    }
    
    uint8_t port_mask = 1 << port;
    
    if ((OPT_PORTS[id >> 4] & port_mask) != port_mask) {
        return;
    }
    
    
}

void opts_service(void) {
    
}