#include <xc.h>
#include "ui.h"
#include "ui_game.h"
#include "../../rng.h"
#include "../../game.h"
#include "../../edgework.h"
#include "../../buzzer.h"
#include "../../peripherals/lcd.h"
#include "../../tick.h"

#define GAME_RNG_MASK 0x89b1a96c

uint8_t ui_game_quick_start(uint8_t current, action_t *a) {
    uint32_t seed = rng_generate(&base_seed, GAME_RNG_MASK);

    this_module->enabled = true;

    /* Create the game. */
    game_create(seed, 3, 5, 0);

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

    uint8_t *title = "Edgework X/X";
    edgework_display(ui_game_edgework);
    ui_render_menu_item_text(title, false, has_left, has_right);

    lcd_number(0, 11, 1, ui_game_edgework + 1);
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