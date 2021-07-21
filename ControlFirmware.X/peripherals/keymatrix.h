#ifndef KEYMATRIX_H
#define	KEYMATRIX_H

#include "ports.h"

void keymatrix_initialise(pin_t *cols, pin_t *rows, uint8_t mode);
void keymatrix_service(void);
uint8_t keymatrix_fetch(void);
void keymatrix_clear(void);

#define KEYMODE_COL_TO_ROW 0
#define KEYMODE_COL_ONLY   1

#define KEY_ROW_BITS 0b00111000
#define KEY_COL_BITS 0b00000111
#define KEY_NUM_BITS (KEY_ROW_BITS | KEY_COL_BITS)
#define KEY_DOWN_BIT 0b10000000

#define KEY_NO_PRESS 0b11111111

#endif	/* KEYMATRIX_H */

