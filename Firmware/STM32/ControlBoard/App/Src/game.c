#include "game.h"

#include <stdint.h>

#include "mode.h"
#include "sys/sys_clock.h"

typedef struct {
    uint32_t seed;

    uint8_t mode;
    uint8_t state;

    uint8_t strikes;
    uint8_t strikes_allowed;

    struct {
        uint32_t initial_time_in_us;
        uint32_t time_in_us;
    } clock;

    struct {
        uint16_t active;
        uint16_t puzzle;
        uint16_t needy;
        uint16_t solved;
    } node_masks;
} game_t;

static game_t game = {0};

void Game_Init(void) {
}

void Game_Service(void) {
}

bool Game_IsController(void) {
    return Mode_Get() == MODE_SUPPORT_CHASSIS;
}

void Game_SendEvent(const Game_EventType event) {
    Packet packet = {0};
    packet.game.event.event = (uint8_t) event;
    packet.game.event.timestamp = game.clock.time_in_us;
    Protocol_Send(GAME_EVENT, &packet);
}

void Game_ReceiveRequestTransition(Protocol_Message *message) {
    if (!Game_IsController()) {
        return;
    }
}

void Game_ReceiveUpdate(Protocol_Message *message) {
    if (Game_IsController()) {
        return;
    }

    if (game.state != message->packet->game.update.state) {
        /* Transition? */
    }

    game = (game_t) {
        .seed = message->packet->game.update.seed,
        .mode = message->packet->game.update.mode,
        .state = message->packet->game.update.state,
        .strikes = message->packet->game.update.strikes,
        .strikes_allowed = message->packet->game.update.strikes_allowed,
        .clock = {
            .initial_time_in_us = message->packet->game.update.clock.initial_time_in_us,
            .time_in_us = message->packet->game.update.clock.time_in_us,
        },
        .node_masks = {
            .active = message->packet->game.update.node_masks.active,
            .puzzle = message->packet->game.update.node_masks.puzzle,
            .needy = message->packet->game.update.node_masks.needy,
            .solved = message->packet->game.update.node_masks.solved,
        },
    };
}

void Game_ReceiveEvent(Protocol_Message *message) {
    if (!Game_IsController()) {
        return;
    }


}
