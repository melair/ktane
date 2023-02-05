#include <xc.h>
#include "keys.h"
#include "../../peripherals/timer/segment.h"
#include "../../peripherals/ports.h"
#include "../../peripherals/keymatrix.h"
#include "../../mode.h"
#include "../../game.h"
#include "../../argb.h"
#include "../../buzzer.h"
#include "../../rng.h"
#include "../../tick.h"
#include "../needy.h"

#define KEYS_RNG_MASK 0xe3236543

void keys_service(void);
void key_create_next(void);
void key_service_running(bool first);
void key_service_setup(bool first);

/* Keymatrix. */
pin_t keys_cols[] = {KPIN_B0, KPIN_B1, KPIN_B2, KPIN_NONE};
pin_t keys_rows[] = {KPIN_NONE};

void keys_initialise(void) {
    /* Initialise the seven segment display. */
    segment_initialise();
    
    /* Register state service handlers with mode. */
    mode_register_callback(GAME_ALWAYS, keys_service, NULL);
    mode_register_callback(GAME_RUNNING, key_service_running, &tick_20hz);
    mode_register_callback(GAME_SETUP, key_service_setup, &tick_20hz);
    
    /* Initialise keymatrix. */
    keymatrix_initialise(&keys_cols[0], &keys_rows[0], KEYMODE_COL_ONLY);
}

void keys_service(void) {
    segment_service();
    keymatrix_service();
}

const uint8_t led_index[3] = {6,4,2};

void key_create_next(void) {
    uint16_t delay = KEYS_MIN_INTERVAL + (rng_generate8(&game.module_seed, KEYS_RNG_MASK) % (KEYS_MAX_INTERVAL - KEYS_MIN_INTERVAL));    
    uint16_t key_at = (game.time_remaining.minutes * 60) + game.time_remaining.seconds;
    
    if (key_at <= delay) {
        key_at = 1;
    } else {
        key_at -= delay;
    }
    
    mode_data.keys.seconds = key_at % 60;
    mode_data.keys.minutes = (key_at - mode_data.keys.seconds) / 60;            
    mode_data.keys.key = rng_generate8(&game.module_seed, KEYS_RNG_MASK) % 3;
    
    uint8_t minutes = mode_data.keys.minutes % 10;
    uint8_t tenminutes = mode_data.keys.minutes / 10;
    
    uint8_t seconds = mode_data.keys.seconds % 10;
    uint8_t tenseconds = mode_data.keys.seconds / 10;
    
    segment_set_colon(true);

    segment_set_digit(0, characters[DIGIT_0 + tenminutes]);
    segment_set_digit(1, characters[DIGIT_0 + minutes]);

    segment_set_digit(2, characters[DIGIT_0 + tenseconds]);
    segment_set_digit(3, characters[DIGIT_0 + seconds]);
    
    argb_set(led_index[0], 31, 0, 0, 0);
    argb_set(led_index[1], 31, 0, 0, 0);
    argb_set(led_index[2], 31, 0, 0, 0);
}

void key_service_setup(bool first) {
    if (first) {
        game_module_ready(true);
    }
}

void key_service_running(bool first) {
    if (first) {
        keymatrix_clear();
        key_create_next();
    }
    
    if (needy_all_other_modules_complete()) {
        game_module_solved(true);
        return;
    }
    
    uint16_t time = (game.time_remaining.minutes * 60) + game.time_remaining.seconds;
    uint16_t key_at = (mode_data.keys.minutes * 60) + mode_data.keys.seconds;    
    
    if (time < key_at) {
        game_module_strike(1);
        key_create_next();        
    }
    
    if (time - key_at <= KEYS_LIGHT_WARNING) {
        argb_set(led_index[mode_data.keys.key], 31, 0, 255, 0);
    }
    
    /* Handle moves. */
    for (uint8_t press = keymatrix_fetch(); press != KEY_NO_PRESS; press = keymatrix_fetch()) {
        /* Feedback to user button was accepted. */
        buzzer_on_timed(BUZZER_DEFAULT_VOLUME, BUZZER_FREQ_A6_SHARP, 40);           
            
        /* Map bits from keymatrix into a number. */
        uint8_t key = (press & KEY_COL_BITS);            
        
        if (key != mode_data.keys.key || time != key_at) {
            game_module_strike(1);
            key_create_next();   
            continue;
        }
        
        key_create_next();   
        continue; 
    }
}