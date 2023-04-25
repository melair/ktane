#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include "sound.h"
#include "buzzer.h"
#include "../common/packet.h"

/* Does the module need to play its own sounds? */
bool sound_local = true;

/* Local function prototypes. */
void sound_play_local(uint16_t sound);
void sound_play_remote(uint16_t sound);
void sound_stop_remote(uint16_t sound);

typedef struct {
    uint16_t sound;
    uint16_t frequency;
    uint16_t duration;
} local_sound_t;

#define SOUND_COUNT 17

local_sound_t local_sounds[SOUND_COUNT] = {
    {
        .sound = SOUND_CONTROLLER_BEEP_HIGH,
        .frequency = BUZZER_FREQ_F5_SHARP,
        .duration = 700
    },
    {
        .sound = SOUND_CONTROLLER_BEEP_LOW,
        .frequency = BUZZER_FREQ_D5_SHARP,
        .duration = 700
    },
    {
        .sound = SOUND_CONTROLLER_STRIKE,
        .frequency = BUZZER_DEFAULT_FREQUENCY,
        .duration = 750
    },
    {
        .sound = SOUND_CONTROLLER_GAMEOVER_SUCCESS,
        .frequency = BUZZER_DEFAULT_FREQUENCY,
        .duration = 100
    },
    {
        .sound = SOUND_CONTROLLER_GAMEOVER_FAILURE,
        .frequency = BUZZER_DEFAULT_FREQUENCY,
        .duration = 1000
    },
    {
        .sound = SOUND_SIMON_SAYS_550HZ,
        .frequency = 550,
        .duration = 300
    },
    {
        .sound = SOUND_SIMON_SAYS_660HZ,
        .frequency = 660,
        .duration = 300
    },
    {
        .sound = SOUND_SIMON_SAYS_775HZ,
        .frequency = 775,
        .duration = 300
    },
    {
        .sound = SOUND_SIMON_SAYS_985HZ,
        .frequency = 985,
        .duration = 300

    },
    {
        .sound = SOUND_ALL_PRESS_IN_RELEASE,
        .frequency = BUZZER_FREQ_A6_SHARP,
        .duration = 40
    },
    {
        .sound = SOUND_ALL_PRESS_IN,
    },
    {
        .sound = SOUND_ALL_PRESS_RELEASE,
    },
    {
        .sound = SOUND_ALL_NEEDY_ACTIVATED,
    },
    {
        .sound = SOUND_ALL_NEEDY_WARNING,
        .frequency = BUZZER_FREQ_A6_SHARP,
        .duration = 500
    },
    {
        .sound = SOUND_CARDSCAN_WRONG_CARD,
        .frequency = BUZZER_FREQ_A6_SHARP,
        .duration = 100
    },
    {
        .sound = SOUND_CARDSCAN_CORRECT_CARD,
        .frequency = BUZZER_FREQ_D5_SHARP,
        .duration = 100
    },
    {
        .sound = SOUND_ALL_EDGEWORK_2FA,
        .frequency = BUZZER_FREQ_A6_SHARP,
        .duration = 40
    },
};

void sound_play(uint16_t sound) {
    if (sound_local) {
        sound_play_local(sound);
    } else {
        sound_play_remote(sound);
    }
}

void sound_stop(uint16_t sound) {
    if (!sound_local) {
        sound_stop_remote(sound);
    }
}

void sound_play_local(uint16_t sound) {
    for (uint8_t i = 0; i < SOUND_COUNT; i++) {
        if (local_sounds[i].sound == sound) {
            buzzer_on(BUZZER_DEFAULT_VOLUME, local_sounds[i].frequency, local_sounds[i].duration);
            return;
        }
    }
}

void sound_play_remote(uint16_t sound) {
    packet_outgoing.game.sound_play.sound = sound;
    packet_send(PREFIX_GAME, OPCODE_GAME_SOUND_PLAY, SIZE_GAME_SOUND_PLAY, &packet_outgoing);
}

void sound_stop_remote(uint16_t sound) {
    packet_outgoing.game.sound_stop.sound = sound;
    packet_send(PREFIX_GAME, OPCODE_GAME_SOUND_STOP, SIZE_GAME_SOUND_STOP, &packet_outgoing);
}

void sound_receive_play(uint8_t id, packet_t *p) {

}

void sound_receive_stop(uint8_t id, packet_t *p) {

}