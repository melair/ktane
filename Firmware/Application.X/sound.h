#ifndef SOUND_H
#define	SOUND_H

#include "../common/packet.h"

void sound_initialise(void);

void sound_play(uint16_t sound);
void sound_stop(uint16_t sound);

void sound_receive_play(uint8_t id, packet_t *p);
void sound_receive_stop(uint8_t id, packet_t *p);

#define SOUND_CONTROLLER_BEEP_HIGH              0x0300
#define SOUND_CONTROLLER_BEEP_LOW               0x0301
#define SOUND_CONTROLLER_STRIKE                 0x0302
#define SOUND_CONTROLLER_GAMEOVER_SUCCESS       0x0303
#define SOUND_CONTROLLER_GAMEOVER_FAILURE       0x0304

#define SOUND_SIMON_SAYS_550HZ                  0x0c00
#define SOUND_SIMON_SAYS_660HZ                  0x0c01
#define SOUND_SIMON_SAYS_775HZ                  0x0c02
#define SOUND_SIMON_SAYS_985HZ                  0x0c03

#define SOUND_OPERATOR_BASE                     0x1100

#define SOUND_CARDSCAN_WRONG_CARD               0x1200
#define SOUND_CARDSCAN_CORRECT_CARD             0x1201

#define SOUND_ALL_PRESS_IN_RELEASE              0xff00
#define SOUND_ALL_PRESS_IN                      0xff01
#define SOUND_ALL_PRESS_RELEASE                 0xff02
#define SOUND_ALL_NEEDY_ACTIVATED               0xff03
#define SOUND_ALL_NEEDY_WARNING                 0xff04
#define SOUND_ALL_EDGEWORK_2FA                  0xff05
#define SOUND_ALL_ROTARY_CLICK                  0xff06

#endif	/* SOUND_H */

