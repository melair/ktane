#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include "whosonfirst.h"
#include "../../buzzer.h"
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

/* Dictionary for button words, aggregated string. Actually operates as two
 * groups of words, split in the middle. Thanks TheDarkSid3r. */
const uint8_t *lowerwords = 
    "BLANK   "
    "DONE    "
    "FIRST   "
    "HOLD    "
    "LEFT    "
    "LIKE    "
    "MIDDLE  "
    "NEXT    "
    "NO      "
    "NOTHING "
    "OKAY    "
    "PRESS   "
    "READY   "
    "RIGHT   "
    "SURE    "
    "U       "
    "UHHH    "
    "UH HUH  "
    "UH UH   "
    "UR      "
    "WAIT    "
    "WHAT    "
    "WHAT?   "
    "YES     "
    "YOU     "
    "YOU ARE "
    "YOUR    "
    "YOU'RE  "
;

#define CHOICE_COUNT 14

/* Solution map for each of the lower words, takes nearly 400 bytes of program
 * space - could be compressed into using nibbles. */
const uint8_t choice[LOWERWORDS_COUNT][CHOICE_COUNT] = {
    {23, 10, 21, 6, 4, 11, 13, 0, 12, 8, 2, 16, 9, 20},
    {4, 10, 23, 6, 8, 13, 9, 16, 20, 12, 0, 21, 11, 2},
    {0, 16, 20, 2, 21, 12, 13, 23, 9, 4, 11, 10, 8, 6},
    {20, 13, 10, 6, 0, 11, 12, 9, 8, 21, 4, 16, 23, 2},
    {16, 13, 10, 6, 23, 0, 8, 11, 4, 21, 20, 2, 9, 12},
    {10, 13, 16, 6, 2, 21, 11, 12, 9, 23, 4, 0, 8, 20},
    {16, 21, 4, 9, 12, 0, 6, 8, 10, 2, 20, 23, 11, 13},
    {12, 9, 4, 21, 10, 23, 13, 8, 11, 0, 16, 6, 20, 2},
    {13, 4, 2, 8, 6, 23, 0, 21, 16, 20, 11, 12, 10, 9},
    {23, 9, 12, 11, 8, 20, 21, 13, 6, 4, 16, 0, 10, 2},
    {0, 12, 10, 21, 9, 11, 8, 20, 4, 6, 13, 2, 16, 23},
    {6, 8, 2, 23, 16, 9, 20, 10, 4, 12, 0, 11, 21, 13},
    {16, 8, 0, 10, 23, 4, 2, 11, 21, 20, 9, 12, 13, 6},
    {13, 6, 23, 12, 11, 10, 9, 16, 0, 4, 2, 21, 8, 20},
    {14, 25, 26, 27, 7, 17, 19, 3, 22, 24, 18, 5, 1, 15},
    {26, 7, 5, 17, 22, 1, 18, 3, 24, 15, 27, 14, 19, 25},
    {18, 25, 17, 26, 7, 19, 14, 15, 27, 24, 22, 3, 5, 1},
    {24, 27, 19, 7, 18, 25, 15, 26, 22, 17, 14, 1, 5, 3},
    {1, 15, 19, 17, 22, 14, 26, 3, 27, 5, 7, 18, 25, 24},
    {17, 14, 7, 22, 27, 19, 18, 1, 15, 24, 5, 3, 25, 26},
    {17, 26, 25, 24, 1, 3, 18, 7, 14, 5, 27, 19, 15, 22},
    {19, 15, 25, 27, 7, 18, 1, 24, 17, 5, 26, 14, 3, 22},
    {24, 3, 27, 26, 15, 1, 18, 5, 25, 17, 19, 7, 22, 14},
    {14, 17, 7, 22, 26, 19, 27, 3, 5, 24, 15, 25, 18, 1},
    {22, 17, 18, 26, 3, 14, 7, 5, 1, 25, 19, 27, 15, 24},
    {25, 15, 1, 18, 24, 19, 14, 22, 27, 7, 3, 17, 26, 5},
    {25, 1, 5, 27, 24, 3, 17, 19, 14, 15, 22, 7, 26, 18},
    {27, 7, 15, 19, 3, 1, 18, 22, 17, 24, 5, 14, 25, 26},
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
                    
                    if (mode_data.whosonfirst.stage >= 3) {
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