#ifndef SEGMENT_H
#define	SEGMENT_H

#include <stdbool.h>

void segment_initialise(void);
void segment_service(void);
void segment_set_digit(uint8_t digit, uint8_t value);
void segment_set_colon(bool on);

extern const uint8_t characters[28];

#define DIGIT_SPACE         0
#define DIGIT_0             1
#define DIGIT_1             2
#define DIGIT_2             3
#define DIGIT_3             4
#define DIGIT_4             5
#define DIGIT_5             6
#define DIGIT_S             6
#define DIGIT_6             7
#define DIGIT_7             8
#define DIGIT_8             9
#define DIGIT_9             10
#define DIGIT_A             11
#define DIGIT_B             12
#define DIGIT_C             13
#define DIGIT_D             14
#define DIGIT_E             15
#define DIGIT_F             16
#define DIGIT_H             17
#define DIGIT_L             18
#define DIGIT_P             19
#define DIGIT_OVERSCORE     20
#define DIGIT_DASH          21
#define DIGIT_UNDERSCORE    22
#define DIGIT_THREESCORE    23
#define DIGIT_TOPBOTTOM     24
#define DIGIT_DOUBLE_PIPE   25
#define DIGIT_LEFT_PIPE     26
#define DIGIT_PERIOD        27

#define SEGMENT_A  0b00000010
#define SEGMENT_B  0b10000000
#define SEGMENT_C  0b00100000
#define SEGMENT_D  0b00000100
#define SEGMENT_E  0b00000001
#define SEGMENT_F  0b00001000
#define SEGMENT_G  0b01000000
#define SEGMENT_DP 0b00010000

#endif	/* SEGMENT_H */

