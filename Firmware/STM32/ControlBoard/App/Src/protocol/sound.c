#include "protocol/sound.h"

#include "protocol.h"

void Protocol_SoundPlay(uint16_t request_id, uint8_t mode_id,
                        const char *name, uint8_t volume) {
    Packet packet = {0};
    packet.game.sound_play.request_id = request_id;
    packet.game.sound_play.mode_id = mode_id;
    for (uint8_t index = 0u; index < PROTOCOL_SOUND_NAME_SIZE && name[index] != '\0'; ++index) {
        packet.game.sound_play.name[index] = name[index];
    }
    packet.game.sound_play.volume = volume;
    Protocol_Send(GAME_SOUND_PLAY, &packet);
}

void Protocol_SoundStop(uint16_t request_id, uint16_t fade_ms) {
    Packet packet = {0};
    packet.game.sound_stop.request_id = request_id;
    packet.game.sound_stop.fade_ms = fade_ms;
    Protocol_Send(GAME_SOUND_STOP, &packet);
}
