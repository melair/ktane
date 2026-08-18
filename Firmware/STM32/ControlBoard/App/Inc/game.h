#ifndef GAME_H
#define GAME_H

#include <stdbool.h>
#include "protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GAME_EVENT_STRIKE = 0,
    GAME_EVENT_SOLVED,
} Game_EventType;

void Game_Init(void);

void Game_Service(void);

bool Game_IsController(void);

void Game_SendEvent(Game_EventType event);

void Game_ReceiveRequestTransition(Protocol_Message *message);

void Game_ReceiveUpdate(Protocol_Message *message);

void Game_ReceiveEvent(Protocol_Message *message);

#ifdef __cplusplus
}
#endif

#endif //GAME_H
