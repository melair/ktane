#include <xc.h>
#include <stdbool.h>
#include "simon.h"
#include "../../edgework.h"
#include "../../game.h"
#include "../../mode.h"
#include "../../tick.h"
#include "../../rng.h"
#include "../../buzzer.h"
#include "../../peripherals/ports.h"
#include "../../peripherals/pwmled.h"
#include "../../peripherals/keymatrix.h"

#define SIMON_RNG_MASK 0x944a4c4b

#define SIMON_RED    0
#define SIMON_BLUE   1
#define SIMON_GREEN  2
#define SIMON_YELLOW 3

#define SIMON_DIM    15
#define SIMON_BRIGHT 31

const uint16_t simon_freqs[4] = { 550, 775, 660, 985 };
const uint8_t simon_colours[4][3] = { { 255, 0, 0, }, { 0, 0, 255, }, { 0, 255, 0, }, { 255, 64, 0, } };

const uint8_t simon_map[2][3][4] = {
    { // No Vowel
        { // 0 Strike
            SIMON_BLUE, SIMON_YELLOW, SIMON_GREEN, SIMON_RED,                   
        },
        { // 1 Strike
            SIMON_RED, SIMON_BLUE, SIMON_YELLOW, SIMON_GREEN,
        },
        { // 2 Strike
            SIMON_YELLOW, SIMON_GREEN, SIMON_BLUE, SIMON_RED,
        },        
    },
    { // Vowel
        { // 0 Strike
            SIMON_BLUE, SIMON_RED, SIMON_YELLOW, SIMON_GREEN,
        },
        { // 1 Strike
            SIMON_YELLOW, SIMON_GREEN, SIMON_BLUE, SIMON_RED,
        },
        { // 2 Strike
            SIMON_GREEN, SIMON_RED, SIMON_YELLOW, SIMON_BLUE,
        },
    },
};

/* Keymatrix. */
pin_t simon_cols[] = {KPIN_A4, KPIN_A5, KPIN_A6, KPIN_A7, KPIN_NONE};
pin_t simon_rows[] = {KPIN_NONE};

/* Local function prototypes. */
void simon_service(bool first);
void simon_service_setup(bool first);
void simon_service_running(bool first);
uint8_t simon_map_press(uint8_t pressed);
void simon_display_clear(void);

pin_t simon_led_chs[4] = { KPIN_B0, KPIN_B1, KPIN_B2, KPIN_B3 };

/**
 * Initialise the simon says puzzle.
 */
void simon_initialise(void) {
    pwmled_initialise(KPIN_B4, KPIN_B5, KPIN_B6, &simon_led_chs);
    
    /* Register our callbacks. */
    mode_register_callback(GAME_ALWAYS, simon_service, NULL);   
    mode_register_callback(GAME_SETUP, simon_service_setup, &tick_20hz);   
    mode_register_callback(GAME_RUNNING, simon_service_running, &tick_20hz);       
    
    /* Initialise keymatrix. */
    keymatrix_initialise(&simon_cols[0], &simon_rows[0], KEYMODE_COL_ONLY);
}

/**
 * Service the simon says puzzle. 
 * 
 * @param first true if the service routine is called for the first time
 */
void simon_service(bool first) {
    pwmled_service();
    keymatrix_service();
}

/**
 * Set up the Simon say game, randomise sequence and button count.
 * 
 * @param first true if first call
 */
void simon_service_setup(bool first) {
    if (first) {
        uint32_t seed = game.module_seed;
        mode_data.simon.order = rng_generate(&seed, SIMON_RNG_MASK) & 0xffff;
        mode_data.simon.count = 3 + (rng_generate8(&seed, SIMON_RNG_MASK) % 3);
        
        simon_display_clear();
        game_module_ready(true);
                
        mode_data.simon.next_display = 0;
        mode_data.simon.next_display_stage = 0;
        
        mode_data.simon.next_correct_press = 0;
        mode_data.simon.next_highest_press = 0;
    }
}

/**
 * Handle the running of the game.
 * 
 * @param first true if first called
 */
void simon_service_running(bool first) {       
    if (this_module->solved) {
        return;
    }
    
    if (first) {
        mode_data.simon.next_animate = tick_value + 1000;
    }
    
    /* Handle key presses. */
    for (uint8_t press = keymatrix_fetch(); press != KEY_NO_PRESS; press = keymatrix_fetch()) {
        if (press & KEY_DOWN_BIT) {
            /* Force animation of sequence 2 seconds after button press. */
            simon_display_clear();            
            mode_data.simon.next_animate = tick_value + 2000;
            
            /* Calculate the button that is required next. */
            uint8_t expected = (mode_data.simon.order >> (2*mode_data.simon.next_correct_press)) & 0b11;
            uint8_t remapped = simon_map_press(expected);            
           
            /* Check to see if it matches. */
            if (remapped == (press & KEY_NUM_BITS)) {
                /* Play the tone of the target button. */
                buzzer_on_timed(BUZZER_DEFAULT_VOLUME, simon_freqs[remapped], 300);
                
                mode_data.simon.next_correct_press++;
                                
                mode_data.simon.next_display = 0;
                mode_data.simon.next_display_stage = 0;
                mode_data.simon.next_animate += 1600; 

                /* Track the highest button we've pressed this attempt. */
                if (mode_data.simon.next_correct_press > mode_data.simon.next_highest_press) {
                    mode_data.simon.next_highest_press = mode_data.simon.next_correct_press;
                    mode_data.simon.next_correct_press = 0;

                    /* End the game if sequence is completed. */
                    if (mode_data.simon.next_highest_press >= mode_data.simon.count) {
                        game_module_solved(true);
                    }
                }                                 
            } else {
                /* Incorrect button, reset everything to scratch. */
                game_module_strike(1);
                mode_data.simon.next_correct_press = 0;
                mode_data.simon.next_animate = tick_value + 4750;
                mode_data.simon.next_highest_press = 0;
            }
        }       
    }
    
    /* Are we due to animate?. */
    if (tick_value >= mode_data.simon.next_animate) {
        /* Stage 0 is to display button, and beep. */
        if (mode_data.simon.next_display_stage == 0) {
            uint8_t next = (mode_data.simon.order >> (2*mode_data.simon.next_display)) & 0b11;
            
            pwmled_set(next, SIMON_BRIGHT, simon_colours[next][0], simon_colours[next][1], simon_colours[next][2]);
            buzzer_on_timed(BUZZER_DEFAULT_VOLUME, simon_freqs[next], 300);
            
            mode_data.simon.next_display++;            
            
            /* Limit display to buttons seen from player so far. */
            if (mode_data.simon.next_display > mode_data.simon.next_highest_press) {
                mode_data.simon.next_display = 0;
            }
            
            mode_data.simon.next_animate += 300;
            mode_data.simon.next_display_stage = 1;
        } else {
            /* Stage 1, clear button display. */
            simon_display_clear();
            
            /* Limit delay between lights in sequence. */
            if (mode_data.simon.next_display == 0) {
                mode_data.simon.next_animate += 4750;   
            } else {
                mode_data.simon.next_animate += 1000;   
            }
            mode_data.simon.next_display_stage = 0;
        }        
    }
}

/**
 * Clear RGB leds colours back to dim.
 */
void simon_display_clear(void) {
    pwmled_set(0, SIMON_DIM, simon_colours[SIMON_RED][0], simon_colours[SIMON_RED][1], simon_colours[SIMON_RED][2]);
    pwmled_set(1, SIMON_DIM, simon_colours[SIMON_BLUE][0], simon_colours[SIMON_BLUE][1], simon_colours[SIMON_BLUE][2]);
    pwmled_set(2, SIMON_DIM, simon_colours[SIMON_GREEN][0], simon_colours[SIMON_GREEN][1], simon_colours[SIMON_GREEN][2]);
    pwmled_set(3, SIMON_DIM, simon_colours[SIMON_YELLOW][0], simon_colours[SIMON_YELLOW][1], simon_colours[SIMON_YELLOW][2]); 
}

/**
 * Map to expected button, taking into account vowel presence and strikes count.
 * 
 * @param pressed button number pressed
 * @return mapped button
 */
uint8_t simon_map_press(uint8_t pressed) {
    bool vowel = (edgework_serial_vowel() ? 1 : 0);
    uint8_t strikes = (game.strikes_current > 1 ? 2 : game.strikes_current);    
    return simon_map[vowel][strikes][pressed];
}