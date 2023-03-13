#include <xc.h>
#include "opts.h"
#include "nvm.h"
#include "opts/spi/spi.h"
#include "opts/spi/spi_data.h"

#define OPT_PORT_COUNT 4

opts_data_t opts_data[OPT_PORT_COUNT];

void opts_initialise_port(uint8_t port);

#define OPT_TYPE_MASK   0b11110000
#define OPT_DATA_MASK   0b00001111

#define OPT_NONE        0b00000000
#define OPT_PSU         0b00000001
#define OPT_SPI         0b00000010
#define OPT_AUDIO       0b00000011

const uint8_t OPT_PORTS[4] = { 
    0b00000000, 
    0b00001000, // PSU is valid in KPORTD only, I2C.
    0b00001100, // SPI is valid in KPORTC/D, SPI1 on D, SPI2 on C. 
    0b00000001, // AUDIO is valid in KPORTA only, DAC.
};

void opts_initialise(void) {
    for (uint8_t port = 0; port < OPT_PORT_COUNT; port++) {
        uint8_t setting = nvm_read(EEPROM_LOC_OPT_KPORTA + port);

        opts_data[port].type = (setting & OPT_TYPE_MASK) >> 4;
        opts_data[port].data = setting & OPT_DATA_MASK;

        opts_data[2].type = OPT_SPI;
        
        opts_initialise_port(port);
    }
}

void opts_initialise_port(uint8_t port) {     
    opts_data[port].port = port;
    
    if (opts_data[port].type == OPT_NONE) {
        return;
    }
    
    uint8_t port_mask = 1 << port;
    
    if ((OPT_PORTS[opts_data[port].type] & port_mask) != port_mask) {
        opts_data[port].type = OPT_NONE;
        return;
    }
    
    switch(opts_data[port].type) {
        case OPT_PSU:                
            // opt_psu_initialise(&opts_data_t[port]);
            break;
        case OPT_SPI:
            opts_spi_initialise(&opts_data[port]);
            break;
        case OPT_AUDIO:
            // opt_audio_initialise(&opts_data_t[port]);
            break;
    }
}

void opts_service(void) {
    for (uint8_t port = 0; port < OPT_PORT_COUNT; port++) {
        switch(opts_data[port].type) {
            case OPT_PSU:                
                // opt_psu_service(&opts_data_t[port]);
                break;
            case OPT_SPI:
                opts_spi_service(&opts_data[port]);
                break;
            case OPT_AUDIO:
                // opt_audio_service(&opts_data_t[port]);
                break;
        }
    }
}
