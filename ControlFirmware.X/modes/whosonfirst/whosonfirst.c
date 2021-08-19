#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include "whosonfirst.h"
#include "../../buzzer.h"
#include "../../argb.h"
#include "../../mode.h"
#include "../../game.h"
#include "../../lcd.h"
#include "../../rng.h"
#include "../../tick.h"
#include "../../peripherals/keymatrix.h"
#include "../../peripherals/ports.h"

#define WHOSONFIRST_RNG_MASK 0x183cc82a

#define TOPWORDS_COUNT 24
#define WORD_WIDTH 8

/* Top row words, used to identify position look up, aggregated as one string. */
const uint8_t *topwords =
    "YES     "
    "FIRST   "
    "DISPLAY "
    "OKAY    "
    "SAYS    "
    "        "
    "BLANK   "
    "NO      "
    "LED     "
    "LEAD    "
    "RED     "
    "REED    "
    "LEED    "
    "HOLD ON "
    "YOU     "
    "YOUR    "
    "YOU'RE  "
    "UR      "
    "THERE   "
    "THEY'RE "
    "THEY ARE"
    "SEE     "
    "C       "
    "CEE     "
;

/* Look up position for the top word, 0-5, 0 = TL, 1 = TR, 2 = ML, etc. */
const uint8_t topword_key[] = {
    2, 1, 5, 1, 5, 4, 3, 5, 2, 5, 3, 4, 4, 5, 3, 3, 3, 0, 5, 4, 2, 5, 1, 5,
};

#define LOWERWORDS_COUNT 28
#define LOWERWORDS_GROUP_COUNT 14
#define STAGE_COUNT 3

/* Dictionary for button words, aggregated string. Actually operates as two
 * groups of words, split in the middle. Thanks TheDarkSid3r. */
const uint8_t *lowerwords = 
    "READY   "
    "FIRST   "
    "NO      "
    "BLANK   "
    "NOTHING "
    "YES     "
    "WHAT    "
    "UHHH    "
    "LEFT    "
    "RIGHT   "
    "MIDDLE  "
    "OKAY    "
    "WAIT    "
    "PRESS   "
    "YOU     "
    "YOU ARE "
    "YOUR    "
    "YOU'RE  "
    "UR      "
    "U       "
    "UH HUH  "
    "UH UH   "
    "WHAT?   "
    "DONE    "
    "NEXT    "
    "HOLD    "
    "SURE    "
    "LIKE    "
;

#define CHOICE_COUNT 14

/* Solution map for each of the lower words, takes nearly 400 bytes of program
 * space - could be compressed into using nibbles. */
const uint8_t choice[LOWERWORDS_COUNT][CHOICE_COUNT] = {
    {5,  11, 6,  10, 8,  13, 9,  3,  0,  2,  1,  7,  4,  12},
    {8,  11, 5,  10, 2,  9,  4,  7,  12, 0,  3,  6,  13, 1},
    {3,  7,  12, 1,  6,  0,  9,  5,  4,  8,  13, 11, 2,  10},
    {12, 9,  11, 10, 3,  13, 0,  4,  2,  6,  8,  7,  5,  1},
    {7,  9,  11, 10, 5,  3,  2,  13, 8,  6,  12, 1,  4,  0},
    {11, 9,  7,  10, 1,  6,  13, 0,  4,  5,  8,  3,  2,  12},
    {7,  6,  8,  4,  0,  3,  10, 2,  11, 1,  12, 5,  13, 9},
    {0,  4,  8,  6,  11, 5,  9,  2,  13, 3,  7,  10, 12, 1},
    {9,  8,  1,  2,  10, 5,  3,  6,  7,  12, 13, 0,  11, 4},
    {5,  4,  0,  13, 2,  12, 6,  9,  10, 8,  7,  3,  11, 1},
    {3,  0,  11, 6,  4,  13, 2,  12, 8,  10, 9,  1,  7,  5},
    {10, 2,  1,  5,  7,  4,  12, 11, 8,  0,  3,  13, 6,  9},
    {7,  2,  3,  11, 5,  8,  1,  13, 6,  12, 4,  0,  9,  10},
    {9,  10, 5,  0,  13, 11, 4,  7,  3,  8,  1,  6,  2,  12},
    {26, 15, 16, 17, 24, 20, 18, 25, 22, 14, 21, 27, 23, 19},
    {16, 24, 27, 20, 22, 23, 21, 25, 14, 19, 17, 26, 18, 15},
    {21, 15, 20, 16, 24, 18, 26, 19, 17, 14, 22, 25, 27, 23},
    {14, 17, 18, 24, 21, 15, 19, 16, 22, 20, 26, 23, 27, 25},
    {23, 19, 18, 20, 22, 26, 16, 25, 17, 27, 24, 21, 15, 14},
    {20, 26, 24, 22, 17, 18, 21, 23, 19, 14, 27, 25, 15, 16},
    {20, 16, 15, 14, 23, 25, 21, 24, 26, 27, 17, 18, 19, 22},
    {18, 19, 15, 17, 24, 21, 23, 14, 20, 27, 16, 26, 25, 22},
    {14, 25, 17, 16, 19, 23, 21, 27, 15, 20, 18, 24, 22, 26},
    {26, 20, 24, 22, 16, 18, 17, 25, 27, 14, 19, 15, 21, 23},
    {22, 20, 21, 16, 25, 26, 24, 27, 23, 15, 18, 17, 19, 14},
    {15, 19, 23, 21, 14, 18, 26, 22, 17, 24, 25, 20, 16, 27},
    {15, 23, 27, 17, 14, 25, 20, 18, 26, 19, 22, 24, 16, 21},
    {17, 24, 19, 18, 25, 23, 21, 22, 20, 14, 27, 26, 15, 16},
};

pin_t whosonfirst_cols[] = {KPIN_A4, KPIN_A5, KPIN_NONE};
pin_t whosonfirst_rows[] = {KPIN_A0, KPIN_A1, KPIN_A2, KPIN_NONE};

/* Local function prototypes. */
void whosonfirst_service(bool first);
void whosonfirst_service_idle(bool first);
void whosonfirst_service_setup(bool first);
void whosonfirst_service_running(bool first);
void whosonfirst_stage_generate_and_display(void);
uint8_t whosonfirst_word_len(uint8_t *s);
void whosonfirst_update_stage_leds(void);

/**
 * Initialise Who's On First.
 */
void whosonfirst_initialise(void) {
    /* Register callbacks for mode. */
    mode_register_callback(GAME_ALWAYS, whosonfirst_service, NULL);
    mode_register_callback(GAME_IDLE, whosonfirst_service_idle, &tick_20hz);
    mode_register_callback(GAME_SETUP, whosonfirst_service_setup, &tick_20hz);
    mode_register_callback(GAME_RUNNING, whosonfirst_service_running, &tick_20hz); 

    /* Initialise keymatrix. */
    keymatrix_initialise(&whosonfirst_cols[0], &whosonfirst_rows[0], KEYMODE_COL_TO_ROW);
}

/**
 * Service the keymatrix every poll. 
 * 
 * @param first true if first time called
 */
void whosonfirst_service(bool first) {
    keymatrix_service();
}

/**
 * On state change to idle, clear the LCD. 
 * 
 * @param first true if first time called
 */
void whosonfirst_service_idle(bool first) {
    if (first) {
        lcd_clear();
    }
}

/**
 * Set up a new game - unlike most games state is not calculated until the
 * running round. As the game may need to generate seven different combinations 
 * (4 strikes, 3 stages), we defer this to the running stage.
 * 
 * @param first true if first time called
 */
void whosonfirst_service_setup(bool first) {
    if (first) {
        mode_data.whosonfirst.stage = 0;
        game_module_ready(true);
    }
}

/**
 * Handle the game running.
 * 
 * @param first true if first time called
 */
void whosonfirst_service_running(bool first) {
    if (first) {
        keymatrix_clear();
        /* Because setup is not done during set up phase, we must do this to
         * initialise the game. */
        whosonfirst_stage_generate_and_display();
    }
    
    if (this_module->solved) {
        return;
    }
    
    /* Handle selection of button. */
    for (uint8_t press = keymatrix_fetch(); press != KEY_NO_PRESS; press = keymatrix_fetch()) {
        if (press & KEY_DOWN_BIT) {                  
            /* Map the rows/cols to be 0-5. */
            uint8_t mapped = (((press & KEY_ROW_BITS) >> 3) * 2) + (press & KEY_COL_BITS);            
            
            /* Using the top word, identify which button to use for the correct
             * order. */
            uint8_t choicelookup = mode_data.whosonfirst.words[topword_key[mode_data.whosonfirst.topword]];
            
            /* Loop through choices. */
            for (uint8_t i = 0; i < CHOICE_COUNT; i++) {
                /* Check to see if the button pressed is this choice, if so
                 * handle success correctly. */
                if (choice[choicelookup][i] == mapped) {                    
                    mode_data.whosonfirst.stage++;
                    
                    if (mode_data.whosonfirst.stage >= STAGE_COUNT) {
                        whosonfirst_update_stage_leds();
                        game_module_solved(true);
                    } else {
                        whosonfirst_stage_generate_and_display();   
                    }
                } else {                 
                    /* However if it's not, then loop through the button words,
                     * if we match then the player should have pressed that. So
                     * this is the failure state.
                     * 
                     * First glance is this would not catch incorrect presses,
                     * but because there is always a solution we will eventually
                     * evaluate a choice that is present on a button.
                     */
                    for (uint8_t j = 0; j < WHOSONFIRST_WORD_COUNT; j++) {
                        if (mode_data.whosonfirst.words[j] == choice[choicelookup][i]) {
                            game_module_strike(1);
                            whosonfirst_stage_generate_and_display();
                        }
                    }
                }
            }            
        }
    }
}

/**
 * Select the words for the game, and display.
 */
void whosonfirst_stage_generate_and_display(void) {
    /* Choose a top word. */
    mode_data.whosonfirst.topword = rng_generate8(&game.module_seed, WHOSONFIRST_RNG_MASK) % TOPWORDS_COUNT;
    
    /* Select if we're using word group 0 or 1. */
    bool group_zero = (rng_generate8(&game.module_seed, WHOSONFIRST_RNG_MASK) % 2) == 0;
    
    /* Select words for buttons.*/
    for (uint8_t i = 0; i < WHOSONFIRST_WORD_COUNT; i++) {
        bool notunique = true;
        
        /* Loop through each button until it does not match an existing one. */
        while(notunique) {
            /* Select a word, toggling the group as required. */
            mode_data.whosonfirst.words[i] = rng_generate8(&game.module_seed, WHOSONFIRST_RNG_MASK) % LOWERWORDS_GROUP_COUNT;
            if (!group_zero) {
                mode_data.whosonfirst.words[i] += LOWERWORDS_GROUP_COUNT;
            }          
            
            notunique = false;
            
            for (uint8_t j = 0; j < i; j++) {
                if (mode_data.whosonfirst.words[i] == mode_data.whosonfirst.words[j]) {
                    notunique = true;
                }
            }
        }
    }

    /* Display the words on the LCD screen. */
    uint8_t topwidth = whosonfirst_word_len(&topwords[mode_data.whosonfirst.topword * 8]);
    uint8_t topcenter = (20-topwidth) / 2;
    
    lcd_update(0, topcenter, topwidth, &topwords[mode_data.whosonfirst.topword * 8]);
    
    for (uint8_t i = 0; i < WHOSONFIRST_WORD_COUNT; i++) {
        uint8_t r = 1 + (i/2);
        
        uint8_t bottomwidth = whosonfirst_word_len(&lowerwords[mode_data.whosonfirst.words[i] * 8]);
        
        /* Justify the text to the side it is on. */
        if (i % 2 == 0) {
            lcd_update(r, 0, bottomwidth, &lowerwords[mode_data.whosonfirst.words[i] * 8]);
        } else {
            lcd_update(r, 20 - bottomwidth, bottomwidth, &lowerwords[mode_data.whosonfirst.words[i] * 8]);
        }        
    }    
    
    lcd_sync();
    whosonfirst_update_stage_leds();
}

/**
 * Update ARGB strip to show which Who's On First stage is being worked on.
 */
void whosonfirst_update_stage_leds(void) {
    for (uint8_t i = 0; i < STAGE_COUNT; i++) {
        if (i <= mode_data.whosonfirst.stage) {
            argb_set(1+i, 31, 255, 0, 0);
        } else {
            argb_set(1+i, 31, 0, 255, 0);
        }
    }
}

/**
 * Count the number of characters in a word to be displayed, this is needed as
 * we do not use NULL terminated strings, our words are either terminated by a
 * space (0x20) or are 8 characters.
 * 
 * @param s string to get length of
 * @return length of string
 */
uint8_t whosonfirst_word_len(uint8_t *s) {
    for (uint8_t i = 0; i < WORD_WIDTH; i++) {
        if (s[i] == ' ') {
            return i;
        }
    }
    
    return WORD_WIDTH;
}