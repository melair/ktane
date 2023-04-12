#include <xc.h>
#include "opts.h"
#include "../common/nvm.h"
#include "../common/eeprom_addrs.h"
#include "opts/audio/mux.h"
#include "opts/audio/audio.h"
#include "opts/spi/spi.h"
#include "opts/spi/spi_data.h"

#define OPT_PORT_COUNT 4

opt_data_t opts_data[OPT_PORT_COUNT];

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
        uint8_t setting = nvm_eeprom_read(EEPROM_LOC_OPT_KPORTA + port);

        opts_data[port].type = (setting & OPT_TYPE_MASK) >> 4;
        opts_data[port].data = setting & OPT_DATA_MASK;

        opts_initialise_port(port);
    }

    if (opts_find_audio() != NULL && opts_find_sdcard() != NULL) {
        mux_initialise();
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

    switch (opts_data[port].type) {
        case OPT_PSU:
            // opt_psu_initialise(&opts_data_t[port]);
            break;
        case OPT_SPI:
            opts_spi_initialise(&opts_data[port]);
            break;
        case OPT_AUDIO:
            audio_initialise(&opts_data[port]);
            break;
    }
}

void opts_service(void) {
    for (uint8_t port = 0; port < OPT_PORT_COUNT; port++) {
        switch (opts_data[port].type) {
            case OPT_PSU:
                // opt_psu_service(&opts_data_t[port]);
                break;
            case OPT_SPI:
                opts_spi_service(&opts_data[port]);
                break;
            case OPT_AUDIO:
                audio_service(&opts_data[port]);
                mux_service();
                break;
        }
    }
}

opt_data_t *opts_find_audio(void) {
    for (uint8_t port = 0; port < OPT_PORT_COUNT; port++) {
        if (opts_data[port].type == OPT_AUDIO) {
            return &opts_data[port];
        }
    }

    return NULL;
}

opt_data_t *opts_find_rtc(void) {
    for (uint8_t port = 0; port < OPT_PORT_COUNT; port++) {
        if (opts_data[port].type == OPT_SPI && opts_data[port].spi.present.rtc) {
            return &opts_data[port];
        }
    }

    return NULL;
}

opt_data_t *opts_find_sdcard(void) {
    for (uint8_t port = 0; port < OPT_PORT_COUNT; port++) {
        if (opts_data[port].type == OPT_SPI && opts_data[port].spi.present.sd) {
            return &opts_data[port];
        }
    }

    return NULL;
}

opt_data_t *opts_find_nf24(void) {
    for (uint8_t port = 0; port < OPT_PORT_COUNT; port++) {
        if (opts_data[port].type == OPT_SPI && opts_data[port].spi.present.nf24) {
            return &opts_data[port];
        }
    }

    return NULL;
}