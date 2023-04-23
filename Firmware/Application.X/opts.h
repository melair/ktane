#ifndef OPTS_H
#define	OPTS_H

void opts_initialise(void);
void opts_service(void);
uint8_t opts_get(uint8_t port);

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

#include "../common/packet.h"

void opts_receive_opt_set(uint8_t id, packet_t *p);

#define OPT_COUNT       3
#define OPT_MAX_NAME    5

extern const char opts_name[OPT_COUNT][OPT_MAX_NAME];

#endif	/* OPTS_H */

