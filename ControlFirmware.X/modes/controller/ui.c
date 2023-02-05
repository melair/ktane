#include <xc.h>
#include "ui.h"
#include "../../peripherals/rotary.h"
#include "../../peripherals/keymatrix.h"
#include "../../lcd.h"

/* Keymatrix. */
pin_t controller_cols[] = {KPIN_B2, KPIN_NONE};
pin_t controller_rows[] = {KPIN_NONE};

void ui_initialise(void) {
    /* Initialise the rotary encoder. */
    rotary_initialise(KPIN_B0, KPIN_B1);
    /* Initialise keymatrix. */
    keymatrix_initialise(&controller_cols[0], &controller_rows[0], KEYMODE_COL_ONLY);
}

void ui_service(void) {
    /* Service rotary encoder. */
    rotary_service();    
    /* Service keymatrix. */
    keymatrix_service();
    
    /* Handle rotary encoder being turned. */
    int8_t turns = rotary_fetch_delta();
    rotary_clear();
    
    /* Handle rotary encoder being pressed. */
    for (uint8_t press = keymatrix_fetch(); press != KEY_NO_PRESS; press = keymatrix_fetch()) {
        if (press & KEY_DOWN_BIT) {
            //protocol_module_reset_send();
        }
    } 
}


/* Game end screen. 
 
         
        lcd_clear();
        
        uint8_t *text_strikes = "Strikes: X of X";
        uint8_t *text_remaining = "Time: X:XX.XX";        
        
        lcd_update(0, 0, 15, text_strikes);
        lcd_number(0, 9, 1, game.strikes_current);
        lcd_number(0, 14, 1, game.strikes_total);
        
        lcd_update(1, 0, 13, text_remaining);        
        lcd_number(1, 6, 1, game.time_remaining.minutes);        
        lcd_number(1, 8, 2, game.time_remaining.seconds);        
        lcd_number(1, 11, 2, game.time_remaining.centiseconds);
        
        lcd_sync();
 
 */