#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include "keymatrix.h"
#include "../tick.h"
#include "../peripherals/ports.h"

#define _XTAL_FREQ 64000000

/* How many keys to buffer before user picks it up. */
#define KEY_HISTORY 8
/* Key buffer to store key presses. */
uint8_t key_buffer[KEY_HISTORY];
/* Pointer where to read from the buffer next. */
uint8_t read = 0;
/* Pointer where to write to the buffer next. */
uint8_t write = 0;
/* Number of consequitive periods of change that must match for state change, default 5 as 50ms. */
uint8_t required_periods_of_change = 5;

#define KEYMATRIX_MAX_COLS 4
#define KEYMATRIX_MAX_ROWS 4

/* Keymatrix configuration and state. */
pin_t *keymatrix_cols;
pin_t *keymatrix_rows;
uint8_t keymatrix_state[KEYMATRIX_MAX_COLS * KEYMATRIX_MAX_ROWS];
uint8_t keymatrix_mode;

/* Local function prototypes. */
void keymatrix_update_key(uint8_t key, uint8_t row, uint8_t col, bool this_read);
void keymatrix_publish_key(uint8_t row, uint8_t col, bool down);
void keymatrix_service_col_to_row(void);
void keymatrix_service_col_only(void);
void keymatrix_init_col_to_row(void);
void keymatrix_init_col_only(void);

/**
 * Initialise the keymatrix handler.
 * 
 * Key matrix assumes that the column line will be read, thus anti-ghosting
 * diode should be placed with that in mind. Column invert can be used if
 * there are no row pins, and the pins are pulled high.
 * 
 * Columns are expected to be INPUT, rows are expected to by OUTPUT.
 */
void keymatrix_initialise(pin_t *cols, pin_t *rows, uint8_t mode) {
    keymatrix_cols = cols;
    keymatrix_rows = rows;
    keymatrix_mode = mode;
    
    switch(keymatrix_mode) {
        case KEYMODE_COL_TO_ROW:
            keymatrix_init_col_to_row();
            break;
        case KEYMODE_COL_ONLY:
            keymatrix_init_col_only();            
            break;
    }
}

/**
 * Service the key matrix, at 100Hz evaluate all pins being monitored.
 */
void keymatrix_service(void) {
    if (!tick_100hz) {
        return;
    }   
    
    switch(keymatrix_mode) {
        case KEYMODE_COL_TO_ROW:
            keymatrix_service_col_to_row();
            break;
        case KEYMODE_COL_ONLY:
            keymatrix_service_col_only();            
            break;
    }   
}

void keymatrix_init_col_to_row(void) {
    for (uint8_t r = 0; r < KEYMATRIX_MAX_ROWS && keymatrix_rows[r] != KPIN_NONE; r++) {
        kpin_write(keymatrix_rows[r], false);
        kpin_mode(keymatrix_rows[r], PIN_INPUT, true);      
    }
    
    for (uint8_t c = 0; c < KEYMATRIX_MAX_COLS && keymatrix_cols[c] != KPIN_NONE; c++) {
        kpin_mode(keymatrix_cols[c], PIN_INPUT, true);      
    }
}

void keymatrix_service_col_to_row(void) {
    bool this_read;
    uint8_t key = 0;
    
    for (uint8_t r = 0; r < KEYMATRIX_MAX_ROWS && keymatrix_rows[r] != KPIN_NONE; r++) {
        /* Set row to output and low. */
        kpin_mode(keymatrix_rows[r], PIN_OUTPUT, false);      
        
        __delay_us(1);
                
        for (uint8_t c = 0; c < KEYMATRIX_MAX_COLS && keymatrix_cols[c] != KPIN_NONE; c++) {
            this_read = kpin_read(keymatrix_cols[c]);
            keymatrix_update_key(key, r, c, !this_read);            
            key++;         
        }
        
        /* Set row to input, and pulled high. */
        kpin_mode(keymatrix_rows[r], PIN_INPUT, true);      
    }
}

void keymatrix_init_col_only(void){
    for (uint8_t c = 0; c < KEYMATRIX_MAX_COLS && keymatrix_cols[c] != KPIN_NONE; c++) {
        kpin_mode(keymatrix_cols[c], PIN_INPUT, true);      
    }
}

void keymatrix_service_col_only(void) {
    bool this_read;
    uint8_t key = 0;
    
    for (uint8_t c = 0; c < KEYMATRIX_MAX_COLS && keymatrix_cols[c] != KPIN_NONE; c++) {
        this_read = kpin_read(keymatrix_cols[c]);
        keymatrix_update_key(key, 0, c, !this_read);            
        key++;       
    }    
}

/**
 * Allow the application to set the number of measured periods that must match
 * for a change to be accepted.
 * 
 * @param periods number of periods
 */
void keymatrix_required_periods(uint8_t periods) {
    required_periods_of_change = periods;
}

/**
 * Update key state with results from port read.
 * 
 * @param key key number
 * @param row row read
 * @param col col read
 * @param this_read value of read
 */
void keymatrix_update_key(uint8_t key, uint8_t row, uint8_t col, bool this_read) {
    bool current_state = (keymatrix_state[key] & 0b10000000) == 0b10000000;
    bool last_read = (keymatrix_state[key] & 0b01000000) == 0b01000000;
    uint8_t consecutive_reads = keymatrix_state[key] & 0b00111111;

    if (last_read != this_read) {
        consecutive_reads = 0;
    } else {
        if (consecutive_reads < 0b00111111) {
            consecutive_reads++;
        }

        if (consecutive_reads >= required_periods_of_change) {
            if (current_state != this_read) {
                current_state = this_read;
                consecutive_reads = 0;
                
                keymatrix_publish_key(row, col, current_state);
            }
        }
    }
    
    keymatrix_state[key] = consecutive_reads;
    
    if (this_read) {
        keymatrix_state[key] |= 0b01000000;        
    }
    
    if (current_state) {
        keymatrix_state[key] |= 0b10000000;        
    }
}

/**
 * Publish the key event to the key buffer.
 * 
 * @param row row of button
 * @param col column of button
 * @param down down state
 */
void keymatrix_publish_key(uint8_t row, uint8_t col, bool down) {
    uint8_t event = (col & 0b00000111) | ((row << 3) & 0b00111000);
    if (down) {
        event |= KEY_DOWN_BIT;
    }
    
    key_buffer[write] = event;
    
    write++;
    if (write >= KEY_HISTORY) {
        write = 0;
    }   
    
    /* Handle overflowing the key history loop, move the read pointer on. */
    if (write == read) {
        read++;
        if (read >= KEY_HISTORY) {
            read = 0;
        }   
    }    
}

/**
 * Fetch a keymatrix press, a key number can essentially be calculated by AND
 * the result with KEY_NUM_BITS.
 * 
 * DxRRRCCC
 * D = Down/!Up
 * C = Column Bit
 * R = Row Bit
 * 
 * @return key matrix press
 */
uint8_t keymatrix_fetch(void) {
    if (read == write) {
        return KEY_NO_PRESS;
    }
    
    uint8_t ret = key_buffer[read];
    
    read++;
    if (read >= KEY_HISTORY) {
        read = 0;
    }    
    
    return ret;
}

/**
 * Clear any backlog of key presses that have not been fetched.
 */
void keymatrix_clear(void) {
    read = 0;
    write = 0;
}