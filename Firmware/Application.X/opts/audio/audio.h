#ifndef AUDIO_H
#define	AUDIO_H

#include "../../opts.h"
#include "audio_data.h"

#define AUDIO_FRAME_SIZE    500

void audio_initialise(opt_data_t *opt);
void audio_service(opt_data_t *opt);
void audio_register_callback(opt_audio_t *a, void (*callback)(opt_audio_t *a));

#endif	/* AUDIO_H */

