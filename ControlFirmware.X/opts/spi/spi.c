#include <xc.h>
#include "spi.h"
#include "spi_data.h"
#include "rtc.h"
#include "sdcard.h"
#include "../../hal/pins.h"
#include "../../hal/spi.h"

bool opts_spi_rtc_present;
bool opts_spi_sd_present;
bool opts_spi_nf24_present;

void opts_spi_initialise(opt_data_t *opt) {   
    opt->spi.pins.rtc_cs = KPORT_BUILD(opt->port, 0);
    opt->spi.pins.sd_cs = KPORT_BUILD(opt->port, 1);
    opt->spi.pins.nf24_cs = KPORT_BUILD(opt->port, 2);
    opt->spi.pins.nf24_ce = KPORT_BUILD(opt->port, 3);
    opt->spi.pins.i_r_q = KPORT_BUILD(opt->port, 4);
    opt->spi.pins.sclk = KPORT_BUILD(opt->port, 5);
    opt->spi.pins.mosi = KPORT_BUILD(opt->port, 6);
    opt->spi.pins.miso = KPORT_BUILD(opt->port, 7);

    kpin_mode(opt->spi.pins.rtc_cs, PIN_OUTPUT, false);
    kpin_mode(opt->spi.pins.sd_cs, PIN_OUTPUT, false);
    kpin_mode(opt->spi.pins.nf24_cs, PIN_OUTPUT, false);
    
    kpin_mode(opt->spi.pins.rtc_cs, PIN_INPUT, false);
    kpin_mode(opt->spi.pins.sd_cs, PIN_INPUT, false);
    kpin_mode(opt->spi.pins.nf24_cs, PIN_INPUT, false);
    
    opt->spi.present.rtc = kpin_read(opt->spi.pins.rtc_cs);
    opt->spi.present.sd = kpin_read(opt->spi.pins.sd_cs);
    opt->spi.present.nf24 = kpin_read(opt->spi.pins.nf24_cs);       
    
    kpin_mode(opt->spi.pins.rtc_cs, PIN_OUTPUT, false);
    kpin_mode(opt->spi.pins.sd_cs, PIN_OUTPUT, false);
    kpin_mode(opt->spi.pins.nf24_cs, PIN_OUTPUT, false);
    kpin_mode(opt->spi.pins.nf24_ce, PIN_OUTPUT, false);
    kpin_mode(opt->spi.pins.i_r_q, PIN_INPUT, false);
    
    kpin_opendrain(opt->spi.pins.rtc_cs, true);
    kpin_opendrain(opt->spi.pins.sd_cs, true);
    kpin_opendrain(opt->spi.pins.nf24_cs, true);
    kpin_opendrain(opt->spi.pins.nf24_ce, true);

    kpin_write(opt->spi.pins.rtc_cs, true);
    kpin_write(opt->spi.pins.sd_cs, true);
    kpin_write(opt->spi.pins.nf24_cs, true); 
    kpin_write(opt->spi.pins.nf24_ce, true);
    
    kpin_mode(opt->spi.pins.sclk, PIN_OUTPUT, false);
    kpin_mode(opt->spi.pins.mosi, PIN_OUTPUT, false);
    kpin_mode(opt->spi.pins.miso, PIN_INPUT, false);
    
    // TODO: OVERRIDE FOR SD CARD TESTING
    opt->spi.present.rtc = false;
    
    if (opt->spi.present.rtc) {
        rtc_initialise(opt);
    }
    
    if (opt->spi.present.sd) {
        sdcard_initialise(opt);
    }
    
    if (opt->spi.present.nf24) {
        
    }
}

void opts_spi_service(opt_data_t *opt) {
    if (opt->spi.present.rtc) {
        rtc_service(opt);
    }
        
    if (opt->spi.present.nf24) {
        
    }
}
