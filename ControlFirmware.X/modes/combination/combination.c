#include <xc.h>
#include <stdbool.h>
#include "combination.h"
#include "../../peripherals/timer/segment.h"
#include "../../peripherals/ports.h"
#include "../../peripherals/rotary.h"
#include "../../peripherals/keymatrix.h"
#include "../../edgework.h"
#include "../../mode.h"
#include "../../module.h"
#include "../../game.h"
#include "../../argb.h"
#include "../../buzzer.h"
#include "../../rng.h"
#include "../../tick.h"

/* Keymatrix. */
pin_t combination_cols[] = {KPIN_B2, KPIN_NONE};
pin_t combination_rows[] = {KPIN_NONE};

void combination_service(void);
void combination_service_running(bool first);
void combination_service_setup(bool first);
void combination_check(void);
void combination_setup(void);

#define COMBINATION_MAX_VALUE 20

/* Current display value. */
int8_t combination_value = 0;
/* Clockwise */
bool combination_clockwise_traveled = false;
/* Anticlockwise */
bool combination_anticlockwise_traveled = false;
/* Current stage. */
uint8_t combination_stage = 0;
/* Expected values */
uint8_t combination_expected[3] = { 0, 0, 0 };
/* Entered values, incase 2fa changes. */
uint8_t combination_entered[2] = { 0, 0 };
/* First direction should be clockwise. */
bool combination_even_clockwise = false;

void combination_initialise(void) {
    /* Initialise the seven segment display and encoder. */
    segment_initialise();
    rotary_initialise(KPIN_B1, KPIN_B0);
    
    /* Initialise keymatrix. */
    keymatrix_initialise(&combination_cols[0], &combination_rows[0], KEYMODE_COL_ONLY);
    
    /* Register state service handlers with mode. */
    mode_register_callback(GAME_ALWAYS, combination_service, NULL);
    mode_register_callback(GAME_RUNNING, combination_service_running, &tick_20hz);
    mode_register_callback(GAME_SETUP, combination_service_setup, &tick_20hz);
}

void combination_service(void) {
    segment_service();
    rotary_service();
    keymatrix_service();
}

void combination_service_setup(bool first) {
    if (first) {
        keymatrix_clear();
        combination_setup();
        game_module_ready(true);
    }
}

void combination_setup(void){
    combination_stage = 0;
    combination_value = ((uint8_t) (game.module_seed & 0xff)) % COMBINATION_MAX_VALUE;
    combination_even_clockwise = ((combination_value / 10) + (combination_value % 10)) % 3 == 0;
    combination_clockwise_traveled = false;
    combination_anticlockwise_traveled = false;
    
    uint8_t module_count = 0;
    uint8_t solved_count = 0;
    
    for (uint8_t i = 0; i < MODULE_COUNT; i++) {
        module_game_t *that_module = module_get_game(i);
        
        if (that_module == NULL) {
            continue;
        }
        
        module_count++;
        
        if (that_module->solved) {
            solved_count++;
        }       
    }
    
    if (!edgework_twofa_present()) {
        combination_expected[0] = edgework_serial_last_digit() + solved_count;
        combination_expected[1] = module_count;
    }        
}

void combination_service_running(bool first) {
    if (this_module->solved) {
        return;
    }

    int8_t delta = rotary_fetch_delta();
    
    if (delta != 0) {
        if (delta > 0) {
            combination_clockwise_traveled = true;
        } else {
            combination_anticlockwise_traveled = true;
        }
    }
    
    combination_value += delta;
    
    if (combination_value < 0) {
        combination_value = COMBINATION_MAX_VALUE - 1;
    } else if (combination_value >= COMBINATION_MAX_VALUE) {
        combination_value = 0;
    }
    
    rotary_clear();
    
    for (uint8_t press = keymatrix_fetch(); press != KEY_NO_PRESS; press = keymatrix_fetch()) {
        if (press & KEY_DOWN_BIT) {
            buzzer_on_timed(BUZZER_DEFAULT_VOLUME, BUZZER_FREQ_A6_SHARP, 40);
            combination_check();
        }
    }
             
    /* Update segment display. */
    uint8_t edge_character = 0;
    
    switch(combination_stage) {
        case 0:
            edge_character = DIGIT_THREESCORE;
            break;
        case 1:
            edge_character = DIGIT_EQUALS;
            break;
        case 2:
            edge_character = DIGIT_UNDERSCORE;
            break;
    }
   
    if (combination_stage == 3) {
        segment_set_digit(1, characters[0]);
        segment_set_digit(2, characters[0]);
    } else {
        uint8_t digit = combination_value / 10;
        segment_set_digit(1, characters[1 + digit]);
        digit = combination_value % 10;
        segment_set_digit(2, characters[1 + digit]);
    }
   
    segment_set_digit(0, characters[edge_character]);
    segment_set_digit(3, characters[edge_character]);
}

void combination_check(void) {
    if (edgework_twofa_present()) {
        combination_expected[0] = edgework_twofa_digit(0) + edgework_twofa_digit(1);
        combination_expected[1] = edgework_twofa_digit(4) + edgework_twofa_digit(5);
    }
         
    combination_expected[2] = (combination_entered[0] + combination_entered[1]) % COMBINATION_MAX_VALUE;
    
    bool correct = false;
    
    /* Skip and strike if both directions, or if neither. */
    if (combination_clockwise_traveled || combination_anticlockwise_traveled) {
        bool shouldBeClockwise = (combination_stage % 2) == (combination_even_clockwise ? 0 : 1);
                       
        /* Skip if wrong direction, stage is 0 indexed. */
        if ((shouldBeClockwise && combination_clockwise_traveled) || (!shouldBeClockwise && combination_anticlockwise_traveled)) {
            /* If we have the correct value, great. */
            if (combination_expected[combination_stage] == combination_value) {
                combination_entered[combination_stage] = combination_value;
                correct = true;
            }
        }
    }
    
    if (correct) {
        combination_stage++;
    } else {
        game_module_strike(1);
        combination_stage = 0;
        combination_value = ((uint8_t) (game.module_seed & 0xff)) % COMBINATION_MAX_VALUE;
        combination_even_clockwise = ((combination_value / 10) + (combination_value % 10)) % 3 == 0;        
    }
    
    combination_clockwise_traveled = false;
    combination_anticlockwise_traveled = false;
    
    if (combination_stage == 3) {
        game_module_solved(true);       
    }
}