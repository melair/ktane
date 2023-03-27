#include <xc.h>
#include "password.h"
#include "../../buzzer.h"
#include "../../mode.h"
#include "../../game.h"
#include "../../peripherals/lcd.h"
#include "../../rng.h"
#include "../../tick.h"
#include "../../peripherals/keymatrix.h"
#include "../../hal/pins.h"

#define PASSWORD_RNG_MASK 0xa654e64d

/* Words in dictionary. */
#define WORD_COUNT 35

/* Dictionary for passwords. */
const uint8_t words[WORD_COUNT][5] = {
  "ABOUT", "AFTER", "AGAIN", "BELOW", "COULD",
  "EVERY", "FIRST", "FOUND", "GREAT", "HOUSE",
  "LARGE", "LEARN", "NEVER", "OTHER", "PLACE",
  "PLANT", "POINT", "RIGHT", "SMALL", "SOUND",
  "SPELL", "STILL", "STUDY", "THEIR", "THERE",
  "THESE", "THING", "THINK", "THREE", "WATER",
  "WHERE", "WHICH", "WORLD", "WOULD", "WRITE",
};

/* Local function prototypes. */
void password_service(bool first);
void password_service_idle(bool first);
void password_service_setup(bool first);
void password_service_running(bool first);
void password_generate(void);
void password_generate_letters(void);
bool password_options_matches_other(void);
void password_copy_and_shuffle(void);
void password_render_display(void);
void password_enable(bool first);
void password_disable(bool first);

/* Keymatrix */
pin_t password_cols[] = {KPIN_B0, KPIN_B1, KPIN_B2, KPIN_B3, KPIN_NONE};
pin_t password_rows[] = {KPIN_B4, KPIN_B5, KPIN_B6, KPIN_B7, KPIN_NONE};

/**
 * Initialise the password puzzle.
 */
void password_initialise(void) {
    /* Initialise the LCD. */
    lcd_initialize();
    
    /* Load the big font into the LCD. */
    lcd_load_big();

    mode_register_callback(GAME_ALWAYS, password_service, NULL);
    mode_register_callback(GAME_IDLE, password_service_idle, &tick_20hz);
    mode_register_callback(GAME_SETUP, password_service_setup, &tick_20hz);
    mode_register_callback(GAME_RUNNING, password_service_running, &tick_20hz);
    mode_register_callback(GAME_ENABLE, password_enable, NULL);
    mode_register_callback(GAME_DISABLE, password_disable, NULL);

    /* Initialise keymatrix. */
    keymatrix_initialise(&password_cols[0], &password_rows[0], KEYMODE_COL_TO_ROW);

    lcd_set_contrast(0x8a);
}

/**
 * Service the password module.
 *
 * @param first true if first run
 */
void password_service(bool first) {
    lcd_service();
    keymatrix_service();
}

void password_enable(bool first) {
     lcd_set_brightness(lcd_get_nominal_brightness());      
}

void password_disable(bool first) {
    lcd_set_brightness(0);
    
    lcd_clear();
    lcd_sync();
}

/**
 * Handle the idle state
 *
 * @param first true if first call
 */
void password_service_idle(bool first) {
    if (first) {
        lcd_clear();
        lcd_sync();
    }
}

/**
 * Set up the password game.
 *
 * @param first true if first call
 */
void password_service_setup(bool first) {
    if (first) {
        password_generate();
        game_module_ready(true);

        /* Preselect the password characters. */
        for (uint8_t i = 0; i < LENGTH; i++) {
            mode_data.password.selected[i] = rng_generate8(&game.module_seed, PASSWORD_RNG_MASK) % LETTER_OPTIONS;
        }
    }
}

void password_service_running(bool first) {
    if (first) {
        keymatrix_clear();
        password_render_display();
    }

    if (this_module->solved) {
        return;
    }

    /* Handle moves. */
    for (uint8_t press = keymatrix_fetch(); press != KEY_NO_PRESS; press = keymatrix_fetch()) {
        if (press & KEY_DOWN_BIT) {
            /* Feedback to user button was accepted. */
            buzzer_on_timed(BUZZER_DEFAULT_VOLUME, BUZZER_FREQ_A6_SHARP, 40);

            /* Map bits from keymatrix into a number. */
            uint8_t button = ((press & KEY_ROW_BITS) >> 3) + ((press & KEY_COL_BITS) * 4);

            if (button < 5) {
                /* Down button pressed.*/
                if (mode_data.password.selected[button] != 0) {
                    mode_data.password.selected[button]--;
                    password_render_display();
                }
            } else if (button < 10) {
                button -= 5;
                /* Up button press. */
                if (mode_data.password.selected[button] < (LETTER_OPTIONS - 1)) {
                    mode_data.password.selected[button]++;
                    password_render_display();
                }
            } else if (button == 10) {
                /* Solve button pressed. */
                bool match = true;

                /* Check to see if each character matches. */
                for (uint8_t i = 0; i < LENGTH; i++) {
                    if (words[mode_data.password.word][i] != mode_data.password.letters[i][mode_data.password.selected[i]]) {
                        match = false;
                        break;
                    }
                }

                if (match) {
                    game_module_solved(true);
                    lcd_clear();
                    lcd_sync();
                } else {
                    game_module_strike(1);
                }
            }
        }
    }
}

/**
 * Render the LCD display to show characters.
 */
void password_render_display(void) {
    lcd_clear();

    for (uint8_t i = 0; i < LENGTH; i++) {
        lcd_update_big(i, (mode_data.password.letters[i][mode_data.password.selected[i]] - 'A' + BIG_FONT_LETTER_BASE));
    }

    lcd_sync();
}

/**
 * Generate and initialise all password letters.
 */
void password_generate(void) {
    mode_data.password.word = rng_generate8(&game.module_seed, PASSWORD_RNG_MASK) % WORD_COUNT;

    do {
        password_generate_letters();
    } while (password_options_matches_other());

    password_copy_and_shuffle();

    lcd_clear();
}

/**
 * Generate the unwanted letters for the password selection.
 */
void password_generate_letters(void) {
    /* Generate characters that do not match the word. */
    for (uint8_t i = 0; i < LENGTH; i++) {
        for (uint8_t j = 0; j < (LETTER_OPTIONS - 1); j++) {
            bool while_dup = true;

            /* Generate a new character that is unique for this column, and does not match word. */
            while(while_dup) {
                mode_data.password.letters[i][j] = (rng_generate8(&game.module_seed, PASSWORD_RNG_MASK) % ALPHABET) + 'A';

                /* Ensure character does not match target word. */
                if (mode_data.password.letters[i][j] == words[mode_data.password.word][i]) {
                    continue;
                }

                /* Ensure generated character is not already in set. */
                while_dup = false;
                for (uint8_t k = 0; k < j; k++) {
                    if (mode_data.password.letters[i][j] == mode_data.password.letters[i][k]) {
                        while_dup = true;
                        break;
                    }
                }
            }
        }
    }
}

/**
 * Check that the letters only match the password we need to find.
 *
 * @return true if letters match another password
 */
bool password_options_matches_other(void) {
    for (uint8_t i = 0; i < WORD_COUNT; i++) {
        if (i == mode_data.password.word) {
            continue;
        }

        uint8_t match_count = 0;

        for (uint8_t j = 0; j < LENGTH; j++) {
            for (uint8_t k = 0; k < (LETTER_OPTIONS - 1); k++) {
                if (mode_data.password.letters[i][k] == words[i][k]) {
                    match_count++;
                    break;
                }
            }
        }

        if (match_count == LENGTH) {
            return true;
        }
    }


    return false;
}

/**
 * Copy the words actual letters into the buffer, and shuffle.
 */
void password_copy_and_shuffle(void) {
    for (uint8_t i = 0; i < LENGTH; i++) {
        mode_data.password.letters[i][LETTER_OPTIONS - 1] = words[mode_data.password.word][i];

        for (uint8_t j = 0; j < LETTER_OPTIONS; j++) {
            uint8_t new_pos = rng_generate8(&game.module_seed, PASSWORD_RNG_MASK) % LETTER_OPTIONS;

            uint8_t t = mode_data.password.letters[i][j];
            mode_data.password.letters[i][j] = mode_data.password.letters[i][new_pos];
            mode_data.password.letters[i][new_pos] = t;
        }
    }
}