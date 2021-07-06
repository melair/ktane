#ifndef KEYMATRIX_H
#define	KEYMATRIX_H

void keymatrix_initialise(volatile uint8_t **col_ports, uint8_t *col_mask, uint8_t *col_invert, volatile uint8_t **row_ports, uint8_t *row_mask, uint8_t *key_state);
void keymatrix_service(void);
uint8_t keymatrix_fetch(void);
void keymatrix_clear(void);

#define KEY_ROW_BITS 0b00000111
#define KEY_COL_BITS 0b00111000
#define KEY_NUM_BITS (KEY_ROW_BITS | KEY_COL_BITS)
#define KEY_DOWN_BIT 0b10000000

#define KEY_NO_PRESS 0b11111111

#endif	/* KEYMATRIX_H */

