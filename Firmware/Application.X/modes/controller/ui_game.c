#include <xc.h>
#include "ui.h"
#include "ui_game.h"
#include "../../rng.h"
#include "../../game.h"
#include "../../edgework.h"
#include "../../buzzer.h"
#include "../../peripherals/lcd.h"
#include "../../tick.h"
#include "../../module.h"

#define GAME_RNG_MASK 0x89b1a96c

uint8_t ui_game_quick_start(uint8_t current, action_t *a) {
    uint32_t seed = rng_generate(&base_seed, GAME_RNG_MASK);

    this_module->enabled = true;

    /* Create the game. */
    game_create(seed, 2, 5, 0, 5);

    return a->index;
}

uint8_t ui_game_force_strike(uint8_t current, action_t *a) {
    game_module_strike(1);
    return current;
}

int8_t ui_game_edgework = 0;

uint8_t ui_game_edgework_change(uint8_t current, action_t *a) {
    if (a->value_direction) {
        if (ui_game_edgework < edgework_count()) {
            ui_game_edgework++;
            buzzer_on_timed(BUZZER_DEFAULT_VOLUME, BUZZER_DEFAULT_FREQUENCY, 10);
        }
    } else {
        if (ui_game_edgework > 0) {
            ui_game_edgework--;
            buzzer_on_timed(BUZZER_DEFAULT_VOLUME, BUZZER_DEFAULT_FREQUENCY, 10);
        }
    }

    return current;
}

#define UI_GAME_EDGEWORK_WITHIN 1000

uint32_t ui_game_edgework_last_press;
uint8_t ui_game_edgework_presses;

uint8_t ui_game_edgework_press(uint8_t current, action_t *a) {
    if (ui_game_edgework_last_press + UI_GAME_EDGEWORK_WITHIN < tick_value) {
        ui_game_edgework_presses = 0;
    }

    ui_game_edgework_presses++;
    ui_game_edgework_last_press = tick_value;

    if (ui_game_edgework_presses == 2) {
        return a->index;
    }

    return current;
}

void ui_game_edgework_display(interface_t *current) {
    bool has_left = (ui_game_edgework > 0);
    bool has_right = (ui_game_edgework < edgework_count());

    lcd_clear();

    edgework_display((uint8_t) ui_game_edgework);
    ui_render_menu_item_text("Edgework X/X", false, has_left, has_right);

    lcd_number(0, 11, 1, (uint8_t) (ui_game_edgework + 1));
    lcd_number(0, 13, 1, edgework_count() + 1);
}

uint8_t ui_game_abandon(uint8_t current, action_t *a) {
    game_set_state(GAME_OVER, RESULT_NONE);
    return a->index;
}

uint8_t ui_game_idle(uint8_t current, action_t *a) {
    game_set_state(GAME_IDLE, RESULT_NONE);
    return a->index;
}

void ui_game_endgame_display(interface_t *current) {
    lcd_clear();

    ui_render_menu_item_text("Strikes: X/X", true, false, false);
    lcd_number(0, 11, 1, game.strikes_current);
    lcd_number(0, 13, 1, game.strikes_total);

    lcd_update(1, 1, 13, "Time: X:XX.XX");
    lcd_number(1, 7, 1, game.time_remaining.minutes);
    lcd_number(1, 9, 2, game.time_remaining.seconds);
    lcd_number(1, 12, 2, game.time_remaining.centiseconds);
}

uint8_t ui_game_custom_module_count = 5;

uint8_t ui_game_custom_module_count_change(uint8_t current, action_t *a) {
    if (a->value_direction) {
        if (ui_game_custom_module_count < MODULE_COUNT) {
            ui_game_custom_module_count++;
            buzzer_on_timed(BUZZER_DEFAULT_VOLUME, BUZZER_DEFAULT_FREQUENCY, 10);
        }
    } else {
        if (ui_game_custom_module_count > 0) {
            ui_game_custom_module_count--;
            buzzer_on_timed(BUZZER_DEFAULT_VOLUME, BUZZER_DEFAULT_FREQUENCY, 10);
        }
    }

    return current;
}

uint8_t ui_game_custom_module_count_press(uint8_t current, action_t *a) {
    if (ui_game_custom_module_count == 0) {
        return a->alt_index;
    } else {
        return a->index;
    }

    return a->alt_index;
}

void ui_game_custom_module_count_display(interface_t *current) {
    bool has_left = (ui_game_custom_module_count > 0);
    bool has_right = (ui_game_custom_module_count < MODULE_COUNT);
    bool has_press = true;

    lcd_clear();
    char *title;

    if (ui_game_custom_module_count == 0) {
        title = "Back";
    } else {
        title = "Module Count";

        lcd_number(1, 7, 2, ui_game_custom_module_count);
    }

    ui_render_menu_item_text(title, has_press, has_left, has_right);
}

#define TIME_MAX 20

uint8_t ui_game_custom_time = 10;

uint8_t ui_game_custom_time_change(uint8_t current, action_t *a) {
    if (a->value_direction) {
        if (ui_game_custom_time < TIME_MAX) {
            ui_game_custom_time++;
            buzzer_on_timed(BUZZER_DEFAULT_VOLUME, BUZZER_DEFAULT_FREQUENCY, 10);
        }
    } else {
        if (ui_game_custom_time > 0) {
            ui_game_custom_time--;
            buzzer_on_timed(BUZZER_DEFAULT_VOLUME, BUZZER_DEFAULT_FREQUENCY, 10);
        }
    }

    return current;
}

uint8_t ui_game_custom_time_press(uint8_t current, action_t *a) {
    if (ui_game_custom_time == 0) {
        return a->alt_index;
    } else {
        return a->index;
    }

    return a->alt_index;
}

void ui_game_custom_time_display(interface_t *current) {
    bool has_left = (ui_game_custom_time > 0);
    bool has_right = (ui_game_custom_time < TIME_MAX);
    bool has_press = true;

    lcd_clear();
    char *title;

    if (ui_game_custom_time == 0) {
        title = "Back";
    } else {
        title = "Time";

        uint8_t minutes = ui_game_custom_time / 2;
        uint8_t seconds = (30 * (ui_game_custom_time % 2));

        lcd_number(1, 6, 2, minutes);
        lcd_update(1, 8, 1, ":");
        lcd_number(1, 9, 2, seconds);
    }

    ui_render_menu_item_text(title, has_press, has_left, has_right);
}

#define STRIKES_MAX 7

int8_t ui_game_custom_strikes = 2;

uint8_t ui_game_custom_strikes_change(uint8_t current, action_t *a) {
    if (a->value_direction) {
        if (ui_game_custom_strikes < STRIKES_MAX) {
            ui_game_custom_strikes++;
            buzzer_on_timed(BUZZER_DEFAULT_VOLUME, BUZZER_DEFAULT_FREQUENCY, 10);
        }
    } else {
        if (ui_game_custom_strikes >= 0) {
            ui_game_custom_strikes--;
            buzzer_on_timed(BUZZER_DEFAULT_VOLUME, BUZZER_DEFAULT_FREQUENCY, 10);
        }
    }

    return current;
}

uint8_t ui_game_custom_strikes_press(uint8_t current, action_t *a) {
    if (ui_game_custom_strikes == -1) {
        return a->alt_index;
    } else {
        return a->index;
    }

    return a->alt_index;
}

void ui_game_custom_strikes_display(interface_t *current) {
    bool has_left = (ui_game_custom_strikes >= 0);
    bool has_right = (ui_game_custom_strikes < STRIKES_MAX);
    bool has_press = true;

    lcd_clear();
    char *title;

    if (ui_game_custom_strikes == -1) {
        title = "Back";
    } else {
        title = "Strikes";

        lcd_number(1, 7, 2, ((uint8_t) ui_game_custom_strikes));
    }

    ui_render_menu_item_text(title, has_press, has_left, has_right);
}

uint8_t ui_game_custom_start_press(uint8_t current, action_t *a) {
    uint32_t seed = rng_generate(&base_seed, GAME_RNG_MASK);

    this_module->enabled = true;

    uint8_t minutes = ui_game_custom_time / 2;
    uint8_t seconds = (30 * (ui_game_custom_time % 2));

    /* Create the game. */
    game_create(seed, ((uint8_t) ui_game_custom_strikes), minutes, seconds, ui_game_custom_module_count);

    return a->index;
}

void ui_game_custom_start_display(interface_t *current) {
    lcd_clear();
    ui_render_menu_item_text("Start Custom", true, false, false);

    uint8_t minutes = ui_game_custom_time / 2;
    uint8_t seconds = (30 * (ui_game_custom_time % 2));

    lcd_number(1, 0, 2, minutes);
    lcd_update(1, 2, 1, ":");
    lcd_number(1, 3, 2, seconds);

    lcd_update(1, 7, 2, "M:");
    lcd_number(1, 9, 2, ui_game_custom_module_count);

    lcd_update(1, 12, 2, "S:");
    lcd_number(1, 14, 2, (uint16_t) ui_game_custom_strikes);
}
