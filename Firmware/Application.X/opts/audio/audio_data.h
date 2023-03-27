#ifndef AUDIO_DATA_H
#define	AUDIO_DATA_H

#include <stdint.h>

#define AUDIO_BUFFER_COUNT 2

typedef struct opt_audio_t opt_audio_t;

struct opt_audio_t {
    uint16_t    size;   
    
    uint8_t     *buffer;
    uint8_t     buffer_next;
    
    void (*callback)(opt_audio_t *);
};

#endif	/* AUDIO_DATA_H */

