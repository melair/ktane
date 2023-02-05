#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include "operator.h"
#include "../../argb.h"
#include "../../edgework.h"
#include "../../game.h"
#include "../../mode.h"
#include "../../peripherals/ports.h"
#include "../../peripherals/keymatrix.h"
#include "../../rng.h"
#include "../../tick.h"

/* Rotary dial as keymatrix. */
pin_t operator_cols[] = {KPIN_A0, KPIN_A1, KPIN_NONE};
pin_t operator_rows[] = {KPIN_NONE};

void operator_service(void);
void operator_service_running(bool first);
void operator_service_setup(bool first);
void operator_display_leds(void);

#define OPERATOR_COL    10
#define OPERATOR_ROW    10
#define OPERATOR_TOTAL  (OPERATOR_COL * OPERATOR_ROW)

#define OPERATOR_RNG_MASK 0xd7e083da

const uint8_t operator_lookup[] = {
    1, 7, 7, 9, 1, 8, 5, 10, 6, 10,
    4, 1, 2, 9, 8, 4, 1, 5, 7, 6,
    5, 6, 8, 8, 7, 7, 7, 8, 10, 5,
    1, 8, 7, 1, 9, 6, 7, 1, 5, 6,
    3, 10, 4, 3, 3, 7, 8, 4, 9, 3,
    7, 1, 9, 9, 10, 5, 8, 8, 3, 5,
    1, 10, 5, 6, 6, 8, 1, 2, 3, 6,
    3, 6, 2, 8, 7, 4, 7, 3, 6, 10,
    3, 3, 7, 3, 1, 9, 3, 3, 1, 2,
    8, 6, 6, 7, 10, 3, 2, 3, 5, 8
};

#define OPERATOR_DIAL_LENGTH 5

void operator_initialise(void) {   
    /* Register state service handlers with mode. */
    mode_register_callback(GAME_ALWAYS, operator_service, NULL);
    mode_register_callback(GAME_RUNNING, operator_service_running, &tick_20hz);
    mode_register_callback(GAME_SETUP, operator_service_setup, &tick_20hz);
    
    /* Initialise keymatrix. */
    keymatrix_initialise(&operator_cols[0], &operator_rows[0], KEYMODE_COL_ONLY);

    /* Rotary dials should have a 66.6ms break period, and 33.3ms make period
     * after. Lowering the required number of sample periods to detect a change
     * to 1 seems wrong, but it would unlikely that we would detect an initial
     * bounce and for it to still be bouncing 10ms later. */
    keymatrix_required_periods(1);
    
    /* Enable the six LEDs. */
    argb_module_leds(6);
}

void operator_service(void) {
    keymatrix_service();      
}

void operator_service_setup(bool first) {
    if (first) {
        bool left = edgework_twofa_present();
        bool reverse = !edgework_serial_vowel();
        
        mode_data.operator.offset_x = rng_generate8(&game.module_seed, OPERATOR_RNG_MASK) % (10 - OPERATOR_LENGTH);
        mode_data.operator.offset_y = rng_generate8(&game.module_seed, OPERATOR_RNG_MASK) % (10 - OPERATOR_LENGTH);
        
        uint8_t xd = (left ? OPERATOR_LENGTH : 1);
        uint8_t yd = (left ? 1 : OPERATOR_LENGTH);        
        
        uint8_t i = 0;
        
        for (uint8_t x = 0; x < xd; x++) {
            for (uint8_t y = 0; y < yd; y++) {
                mode_data.operator.wanted_numbers[i] = operator_lookup[(mode_data.operator.offset_x + x) + ((mode_data.operator.offset_y + y) * 10)];
                i++;
            }
        }
        
        if (reverse) {
            for (uint8_t i = 0; i < (OPERATOR_LENGTH / 2); i++) {
                uint8_t swap = mode_data.operator.wanted_numbers[i];
                mode_data.operator.wanted_numbers[i] = mode_data.operator.wanted_numbers[OPERATOR_LENGTH - 1 - i];
                mode_data.operator.wanted_numbers[OPERATOR_LENGTH - 1 - i] = swap;
            }
        }   
        
        mode_data.operator.offset_x++;
        mode_data.operator.offset_y++;
        
        operator_display_leds();
               
        game_module_ready(true);
    }
}

void operator_display_leds(void) {
    for (uint8_t i = 0; i < 3; i++) {
        bool x_on = mode_data.operator.offset_x & (1 << (2 - i));
        bool y_on = mode_data.operator.offset_y & (1 << (2 - i));

        argb_set(1 + i, 31, 0, 0, (x_on ? 0xff : 0));
        argb_set(1 + 3 + i, 31, 0, 0, (y_on ? 0xff : 0));
    }
}

void operator_service_running(bool first) {
    if (first) {
        keymatrix_clear();        
        mode_data.operator.dialed_pos = 0;
    }
    
    if (this_module->solved) {
        return;
    }
    
    /* Handle rotary changes. */
    for (uint8_t press = keymatrix_fetch(); press != KEY_NO_PRESS; press = keymatrix_fetch()) {            
        /* Down or up? */      
        bool down = (press & KEY_DOWN_BIT);
                    
        /* Map bits from keymatrix into a number. */
        uint8_t key = (press & KEY_COL_BITS);    

        if (key == 0) {
            /* Key matrix works on assumption that pins are down when they are 
             * shorted to GND, however the rotary dial breaks the GND connection
             * when a pulse is sent. So down is actually the release here. */
            if (!down && mode_data.operator.rotary_dialing) {
                mode_data.operator.rotary_pulses++;
            }            
        } else if (key == 1) {
            /* The dialing connection works in line with Keymatrix, so down is
             * the rotary dial being activated. */
            if (down) {
                mode_data.operator.rotary_pulses = 0;
                mode_data.operator.rotary_dialing = true;                
            } else {
                mode_data.operator.rotary_dialing = false;
                
                /* Store last dialed number. */
                mode_data.operator.dialed_numbers[mode_data.operator.dialed_pos] = mode_data.operator.rotary_pulses;
                mode_data.operator.dialed_pos++;
                
                if (mode_data.operator.dialed_pos == OPERATOR_LENGTH) {
                    uint8_t correct = 0;
                    
                    for (uint8_t i = 0; i < OPERATOR_LENGTH; i++) {
                        if (mode_data.operator.dialed_numbers[i] == mode_data.operator.wanted_numbers[i]) {
                            correct++;
                        }
                    }
                    
                    if (correct == OPERATOR_LENGTH) {
                        game_module_solved(true);     
                        
                        for (uint8_t i = 0; i < 6; i++) {            
                            argb_set(1+i, 0, 0, 0, 0);
                        }
                    } else {
                        game_module_strike(1);
                        mode_data.operator.dialed_pos = 0;
                        operator_display_leds();
                    }
                } else {
                    for (uint8_t i = 0; i < 6; i++) {            
                        uint8_t c = (i < mode_data.operator.dialed_pos ? 0xff : 0x00);
                        argb_set(1+i, 31, 0, c, c);
                    }
                }
            }
        }        
    }
}
