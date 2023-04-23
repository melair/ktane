#include <xc.h>
#include "opts.h"
#include "../common/can.h"
#include "../common/packet.h"
#include "../common/nvm.h"
#include "../common/eeprom_addrs.h"
#include "opts/audio/mux.h"
#include "opts/audio/audio.h"
#include "opts/spi/spi.h"
#include "opts/spi/spi_data.h"
#include "opts/power/power.h"

#define OPT_PORT_COUNT 4

opt_data_t opts_data[OPT_PORT_COUNT];

void opts_initialise_port(uint8_t port);

#define OPT_TYPE_MASK   0b11110000
#define OPT_DATA_MASK   0b00001111

#define OPT_NONE        0b00000000
#define OPT_POWER       0b00000001
#define OPT_SPI         0b00000010
#define OPT_AUDIO       0b00000011

const char opts_name[OPT_COUNT][OPT_MAX_NAME] = {
    "Power",
    "SPI",
    "Audio"
};

const uint8_t OPT_PORTS[4] = {
    0b00000111, // NONE is valid on all ports.
    0b00000111, // PSU is valid in all ports with bodge wire.
    0b00000100, // SPI is valid in KPORTC, SPI2 on C.
    0b00000001, // AUDIO is valid in KPORTA only, DAC.
};

void opts_initialise(void) {
    for (uint8_t port = 0; port < OPT_PORT_COUNT; port++) {
        uint8_t setting = nvm_eeprom_read(EEPROM_LOC_OPT_KPORTA + port);

        opts_data[port].type = (setting & OPT_TYPE_MASK) >> 4;
        opts_data[port].data = setting & OPT_DATA_MASK;

        opts_initialise_port(port);
    }

    opt_data_t *audio = opts_find_audio();
    opt_data_t *sdcard = opts_find_sdcard();
    opt_data_t *power = opts_find_power();
    opt_data_t *rtc = opts_find_rtc();
    opt_data_t *nf24 = opts_find_nf24();

    if (audio != NULL && sdcard != NULL) {
        mux_initialise();
    }

    if (power != NULL) {

    }

    if (rtc != NULL) {

    }
}

void opts_initialise_port(uint8_t port) {
    opts_data[port].port = port;

    if (opts_data[port].type == OPT_NONE) {
        return;
    }

    uint8_t port_mask = ((uint8_t) (1 << port));

    if ((OPT_PORTS[opts_data[port].type] & port_mask) != port_mask) {
        opts_data[port].type = OPT_NONE;
        return;
    }

    switch (opts_data[port].type) {
        case OPT_POWER:
            power_initialise(&opts_data[port]);
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
            case OPT_POWER:
                power_service(&opts_data[port]);
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

opt_data_t *opts_find_power(void) {
    for (uint8_t port = 0; port < OPT_PORT_COUNT; port++) {
        if (opts_data[port].type == OPT_POWER) {
            return &opts_data[port];
        }
    }

    return NULL;
}

void opts_receive_opt_set(uint8_t id, packet_t *p) {
    if (p->module.set_opt.can_id != can_get_id()) {
        return;
    }

    uint8_t port_mask = ((uint8_t) (1 << p->module.set_opt.port));

    if ((OPT_PORTS[p->module.set_opt.opt] & port_mask) != port_mask) {
        return;
    }

    nvm_eeprom_write(EEPROM_LOC_OPT_KPORTA + p->module.set_opt.port, ((uint8_t) (p->module.set_opt.opt << 4)));
    RESET();
}

uint8_t opts_get(uint8_t port) {
    return opts_data[port].type;
}