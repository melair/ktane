#ifndef PROTOCOL_SOUND_H
#define PROTOCOL_SOUND_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Send a play request with a caller-assigned request ID and volume 0..255.
 * The name is NUL padded or truncated to the protocol's 10-byte field.
 * Keep request IDs distinct for this sender's active sounds.
 */
void Protocol_SoundPlay(uint16_t request_id, uint8_t mode_id,
                        const char *name, uint8_t volume);

/* Stop this sender's matching request; fade_ms == 0 stops immediately. */
void Protocol_SoundStop(uint16_t request_id, uint16_t fade_ms);

#ifdef __cplusplus
}
#endif

#endif // PROTOCOL_SOUND_H
