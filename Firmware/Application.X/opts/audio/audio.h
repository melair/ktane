#ifndef AUDIO_H
#define	AUDIO_H

#include <stdint.h>
#include "../../opts.h"
#include "audio_data.h"

#define AUDIO_FRAME_SIZE    500

void audio_initialise_always(void);
void audio_initialise(opt_data_t *opt);
void audio_service(opt_data_t *opt);
void audio_register_callback(opt_audio_t *a, void (*callback)(opt_audio_t *a));
void audio_set_volume(uint8_t vol);
uint8_t audio_get_volume(void);

#endif	/* AUDIO_H */

