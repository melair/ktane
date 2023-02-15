#ifndef AUDIO_H
#define	AUDIO_H

#define AUDIO_BUFFER_COUNT 2

typedef struct {
    uint8_t     next;
    uint16_t    size;   
    
    struct {
        uint8_t     *ptr;
        bool        ready;
    } buffer[AUDIO_BUFFER_COUNT];
} audio_t;

void audio_initialise(void);
void audio_service(audio_t *a);
uint8_t *audio_empty_buffer(audio_t *a);
void audio_mark_ready(audio_t *a);

#endif	/* AUDIO_H */

