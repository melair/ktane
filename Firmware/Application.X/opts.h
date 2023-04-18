#ifndef OPTS_H
#define	OPTS_H

void opts_initialise(void);
void opts_service(void);

#include "opts/spi/spi_data.h"
#include "opts/audio/audio_data.h"
#include "opts/power/power_data.h"

typedef struct {
    uint8_t port;
    uint8_t type;
    uint8_t data;
    
    union {
        opt_spi_t spi;
        opt_audio_t audio;
        opt_power_t power;
    };
} opt_data_t;

opt_data_t *opts_find_rtc(void);
opt_data_t *opts_find_sdcard(void);
opt_data_t *opts_find_nf24(void);
opt_data_t *opts_find_audio(void);
opt_data_t *opts_find_power(void);

#endif	/* OPTS_H */

