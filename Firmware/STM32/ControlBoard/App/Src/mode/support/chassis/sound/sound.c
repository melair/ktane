#include "mode/support/chassis/sound/sound.h"

#include "mode.h"
#include "mode/support/chassis/sound/player.h"
#include "nodes.h"
#include "node_link/protocol.h"

_Static_assert(PROTOCOL_SOUND_NAME_SIZE == FAT_NAME_SIZE, "Sound packet and FAT names must match in size");

void Sound_ReceivePlay(Protocol_Message *message) {
    if (Mode_Get() != MODE_SUPPORT_CHASSIS) {
        return;
    }

    const float volume = message->packet->game.sound_play.volume / 255.0f;
    /* Equal-power centre: keep total speaker power constant across columns. */
    float volume_left = volume * 0.70710678f;
    float volume_right = volume_left;
    const uint8_t location = Nodes_GetChassisLocation(message->identifier);
    if (location <= NODE_CHASSIS_LOCATION_SLOT_3) {
        volume_left = volume;
        volume_right = 0.0f;
    } else if (location >= NODE_CHASSIS_LOCATION_SLOT_8 &&
               location <= NODE_CHASSIS_LOCATION_SLOT_11) {
        volume_left = 0.0f;
        volume_right = volume;
    }

    Player_Play(message->identifier,
                message->packet->game.sound_play.request_id,
                message->packet->game.sound_play.mode_id,
                message->packet->game.sound_play.name,
                volume_left, volume_right);
}

void Sound_ReceiveStop(Protocol_Message *message) {
    if (Mode_Get() != MODE_SUPPORT_CHASSIS) {
        return;
    }

    Player_Stop(message->identifier, message->packet->game.sound_stop.request_id,
                message->packet->game.sound_stop.fade_ms);
}
