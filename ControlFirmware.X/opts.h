#ifndef OPTS_H
#define	OPTS_H

void opts_initialise(void);
void opts_service(void);

#include "opts/spi/spi_data.h"

typedef struct {
    uint8_t port;
    uint8_t type;
    uint8_t data;
    
    union {
        opts_spi_t spi;
    };
} opts_data_t;

#endif	/* OPTS_H */

