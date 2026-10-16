#ifndef SOUND_H
#define SOUND_H

#include "protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

void Sound_ReceivePlay(Protocol_Message *message);

void Sound_ReceiveStop(Protocol_Message *message);

#ifdef __cplusplus
}
#endif

#endif // SOUND_H
