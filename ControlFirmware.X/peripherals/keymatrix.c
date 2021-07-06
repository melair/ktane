#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include "keymatrix.h"
#include "../tick.h"

/* How many keys to buffer before user picks it up. */
#define KEY_HISTORY 8
/* Key buffer to store key presses. */
uint8_t key_buffer[KEY_HISTORY];
/* Pointer where to read from the buffer next. */
uint8_t read = 0;
/* Pointer where to write to the buffer next. */
uint8_t write = 0;

/* Number of 100Hz periods required to register a change (debounce time), 50ms. */
#define REQUIRED_PERIODS_OF_CHANGE 5

/* Pointer to array of pointers to column ports. */
volatile uint8_t **keymatrix_col_ports;
/* Array of col bit masks. */
uint8_t *keymatrix_col_mask;
/* Array of col invert masks. */
uint8_t *keymatrix_col_invert;

/* If the key matrix has rows.*/
bool keymatrix_has_rows;
/* Pointer to array of pointers to row ports. */
volatile uint8_t **keymatrix_row_latches;
/* Array of row bit masks. */
uint8_t *keymatrix_row_mask;
/* Pointer to array to store key state in.*/
uint8_t *keymatrix_state;

/* Local function prototypes. */
void keymatrix_update_key(uint8_t key, uint8_t row, uint8_t col, bool this_read);
void keymatrix_publish_key(uint8_t row, uint8_t col, bool down);

/**
 * Initialise the keymatrix handler.
 * 
 * Key matrix assumes that the column line will be read, thus anti-ghosting
 * diode should be placed with that in mind. Column invert can be used if
 * there are no row pins, and the pins are pulled high.
 * 
 * Columns are expected to be INPUT, rows are expected to by OUTPUT.
 * 
 * @param col_ports pointers to the port peripheral that has the pin
 * @param col_mask bit mask to select the column
 * @param col_invert bit mask to xor to potentially invert column
 * @param row_latches pointers to the port peripheral that has the pin
 * @param row_mask bit mask to select the row
 * @param key_state pointer to array in memory to track state, must be col x row large
 */
void keymatrix_initialise(volatile uint8_t **col_ports, uint8_t *col_mask, uint8_t *col_invert, volatile uint8_t **row_latches, uint8_t *row_mask, uint8_t *state) {
   keymatrix_col_ports = col_ports;
   keymatrix_col_mask = col_mask;
   keymatrix_col_invert = col_invert;
   keymatrix_row_latches = row_latches;
   keymatrix_row_mask = row_mask;
   keymatrix_state = state;
   
   keymatrix_has_rows = (keymatrix_row_latches[0] != NULL);
}

/**
 * Service the key matrix, at 100Hz evaluate all pins being monitored.
 */
void keymatrix_service(void) {
    if (!tick_100hz) {
        return;
    }   
    
    uint8_t key = 0;
    
    /* Loop through each column. */
    for (uint8_t c = 0; keymatrix_col_ports[c] != NULL; c++) {       
        bool this_read;
        
        if (keymatrix_has_rows) {
            for (uint8_t r = 0; keymatrix_row_latches[r] != NULL; r++) {                
                /* Set row bit. */
                *keymatrix_row_latches[r] |= keymatrix_row_mask[c];

                this_read = ((*keymatrix_col_ports[c] & keymatrix_col_mask[c]) ^ keymatrix_col_invert[c]) != 0;            
                keymatrix_update_key(key, r, c, this_read);            
                key++;                
                
                /* Clear row bit. */
                *keymatrix_row_latches[r] &= ~keymatrix_row_mask[c];
            }            
        } else {
            this_read = ((*keymatrix_col_ports[c] & keymatrix_col_mask[c]) ^ keymatrix_col_invert[c]) != 0;            
            keymatrix_update_key(key, 0, c, this_read);            
            key++;
        }
    }       
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
        consecutive_reads++;

        if (consecutive_reads >= REQUIRED_PERIODS_OF_CHANGE) {
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