#include <xc.h>
#include "ui.h"
#include "ui_game.h"
#include "ui_configure_module.h"
#include "ui_configure_global.h"
#include "../../peripherals/rotary.h"
#include "../../peripherals/keymatrix.h"
#include "../../buzzer.h"
#include "../../peripherals/lcd.h"
#include "../../tick.h"
#include "../../module.h"
#include "../../sound.h"

/* Keymatrix. */
pin_t ui_cols[] = {KPIN_B2, KPIN_NONE};
pin_t ui_rows[] = {KPIN_NONE};

/* Local prototype functions. */
uint8_t ui_resolve_action(uint8_t current, action_t *action);
uint8_t ui_action_jump(uint8_t current, action_t *a);
void ui_render_menu_item(interface_t *current);
uint8_t ui_action_restart_ktane(uint8_t current, action_t *a);

interface_t interface[] = {
    // 0
    {
        .right =
        {
            .action = &ui_action_jump,
            .index = 1
        },
        .press =
        {
            .action = &ui_action_jump,
            .index = 27
        },
        .render = &ui_render_menu_item,
        .render_data = "Start Game",
        .render_check = &tick_2hz
    },
    // 1
    {
        .left =
        {
            .action = &ui_action_jump,
            .index = 0
        },
        .right =
        {
            .action = &ui_action_jump,
            .index = 2
        },
        .press =
        {
            .action = &ui_action_jump,
            .index = 4
        },
        .render = &ui_render_menu_item,
        .render_data = "Configure",
        .render_check = &tick_2hz
    },
    // 2
    {
        .left =
        {
            .action = &ui_action_jump,
            .index = 1
        },
        .press =
        {
            .action = &ui_action_restart_ktane,
        },
        .render = &ui_render_menu_item,
        .render_data = "Reset KTANE",
        .render_check = &tick_2hz
    },
    // 3
    {
        .right =
        {
            .action = &ui_action_jump,
            .index = 4
        },
        .press =
        {
            .action = &ui_action_jump,
            .index = 1
        },
        .render = &ui_render_menu_item,
        .render_data = "Back",
        .render_check = &tick_2hz
    },
    // 4
    {
        .left =
        {
            .action = &ui_action_jump,
            .index = 3
        },
        .press =
        {
            .action = &ui_action_jump,
            .index = 6
        },
        .right =
        {
            .action = &ui_action_jump,
            .index = 5
        },
        .render = &ui_render_menu_item,
        .render_data = "Modules",
        .render_check = &tick_2hz
    },
    // 5
    {
        .left =
        {
            .action = &ui_action_jump,
            .index = 4
        },
        .press =
        {
            .action = &ui_action_jump,
            .index = 36
        },
        .render = &ui_render_menu_item,
        .render_data = "Global",
        .render_check = &tick_2hz
    },
    // 6
    {
        .left =
        {
            .action = &ui_action_configure_module_select_change,
            .value_direction = false
        },
        .right =
        {
            .action = &ui_action_configure_module_select_change,
            .value_direction = true
        },
        .press =
        {
            .action = &ui_action_configure_module_select_press,
            .index = 8,
            .alt_index = 4
        },
        .render = &ui_render_configure_module_select,
        .render_data = "Select ID",
        .render_check = &tick_2hz
    },
    // 7
    {
        .right =
        {
            .action = &ui_action_jump,
            .index = 8
        },
        .press =
        {
            .action = &ui_action_jump,
            .index = 6
        },
        .render = &ui_render_menu_item,
        .render_data = "Back",
        .render_check = &tick_2hz
    },
    // 8
    {
        .left =
        {
            .action = &ui_action_jump,
            .index = 7
        },
        .right =
        {
            .action = &ui_action_jump,
            .index = 9
        },
        .press =
        {
            .action = &ui_action_jump,
            .index = 16
        },
        .render = &ui_render_menu_item,
        .render_data = "Configure",
        .render_check = &tick_2hz
    },
    // 9
    {
        .left =
        {
            .action = &ui_action_jump,
            .index = 8
        },
        .right =
        {
            .action = &ui_action_jump,
            .index = 10
        },
        .press =
        {
            .action = &ui_action_jump,
            .index = 12
        },
        .render = &ui_render_menu_item,
        .render_data = "Hardware",
        .render_check = &tick_2hz
    },
    // 10
    {
        .left =
        {
            .action = &ui_action_jump,
            .index = 9
        },
        .right =
        {
            .action = &ui_action_jump,
            .index = 11
        },
        .press =
        {
            .action = &ui_action_jump,
            .index = 14
        },
        .render = &ui_render_menu_item,
        .render_data = "CAN State",
        .render_check = &tick_2hz
    },
    // 11
    {
        .left =
        {
            .action = &ui_action_jump,
            .index = 10
        },
        .press =
        {
            .action = &ui_action_jump,
            .index = 13
        },
        .render = &ui_render_menu_item,
        .render_data = "Errors",
        .render_check = &tick_2hz
    },
    // 12
    {
        .left =
        {
            .action = &ui_render_configure_module_hardware_change,
            .value_direction = false
        },
        .right =
        {
            .action = &ui_render_configure_module_hardware_change,
            .value_direction = true
        },
        .press =
        {
            .action = &ui_render_configure_module_hardware_press,
            .alt_index = 9
        },
        .render = &ui_render_configure_module_hardware,
        .render_check = &tick_2hz
    },
    // 13
    {
        .left =
        {
            .action = &ui_render_configure_module_errors_change,
            .value_direction = false
        },
        .right =
        {
            .action = &ui_render_configure_module_errors_change,
            .value_direction = true
        },
        .press =
        {
            .action = &ui_render_configure_module_errors_press,
            .alt_index = 11
        },
        .render = &ui_render_configure_module_errors,
        .render_check = &tick_2hz
    },
    // 14
    {
        .left =
        {
            .action = &ui_render_configure_module_can_stats_change,
            .value_direction = false
        },
        .right =
        {
            .action = &ui_render_configure_module_can_stats_change,
            .value_direction = true
        },
        .press =
        {
            .action = &ui_render_configure_module_can_stats_press,
            .alt_index = 10
        },
        .render = &ui_render_configure_module_can_stats,
        .render_check = &tick_2hz
    },
    // 15
    {
        .right =
        {
            .action = &ui_action_jump,
            .index = 16
        },
        .press =
        {
            .action = &ui_action_jump,
            .index = 8
        },
        .render = &ui_render_menu_item,
        .render_data = "Back",
        .render_check = &tick_2hz
    },
    // 16
    {
        .left =
        {
            .action = &ui_action_jump,
            .index = 15
        },
        .right =
        {
            .action = &ui_action_jump,
            .index = 17
        },
        .press =
        {
            .action = &ui_action_jump,
            .index = 23
        },
        .render = &ui_render_menu_item,
        .render_data = "Mode",
        .render_check = &tick_2hz
    },
    // 17
    {
        .left =
        {
            .action = &ui_action_jump,
            .index = 16
        },
        .right =
        {
            .action = &ui_action_jump,
            .index = 18
        },
        .press =
        {
            .action = &ui_action_jump,
            .index = 44
        },
        .render = &ui_render_menu_item,
        .render_data = "Opt Mods",
        .render_check = &tick_2hz
    },
    // 18
    {
        .left =
        {
            .action = &ui_action_jump,
            .index = 17
        },
        .right =
        {
            .action = &ui_action_jump,
            .index = 19
        },
        .render = &ui_render_menu_item,
        .render_data = "PWM LED Bri",
        .render_check = &tick_2hz
    },
    // 19
    {
        .left =
        {
            .action = &ui_action_jump,
            .index = 18
        },
        .right =
        {
            .action = &ui_action_jump,
            .index = 20
        },
        .render = &ui_render_menu_item,
        .render_data = "LCD Back Bri",
        .render_check = &tick_2hz
    },
    // 20
    {
        .left =
        {
            .action = &ui_action_jump,
            .index = 19
        },
        .right =
        {
            .action = &ui_action_jump,
            .index = 22
        },
        .render = &ui_render_menu_item,
        .render_data = "LCD Contrast",
        .render_check = &tick_2hz
    },
    // 21
    {
        .left =
        {
            .action = &ui_action_jump,
            .index = 27
        },
        .press =
        {
            .action = &ui_action_jump,
            .index = 40
        },
        .render = &ui_render_menu_item,
        .render_data = "Custom",
        .render_check = &tick_2hz
    },
    // 22
    {
        .left =
        {
            .action = &ui_action_jump,
            .index = 20
        },
        .right =
        {
            .action = &ui_action_jump,
            .index = 24
        },
        .render = &ui_render_menu_item,
        .render_data = "DAC/DSP Vol",
        .render_check = &tick_2hz
    },
    // 23
    {
        .left =
        {
            .action = &ui_render_configure_module_mode_set_change,
            .value_direction = false
        },
        .right =
        {
            .action = &ui_render_configure_module_mode_set_change,
            .value_direction = true
        },
        .press =
        {
            .action = &ui_render_configure_module_mode_set_press,
            .alt_index = 16
        },
        .render = &ui_render_configure_module_mode_set,
        .render_check = &tick_2hz
    },
    // 24
    {
        .left =
        {
            .action = &ui_action_jump,
            .index = 22
        },
        .press =
        {
            .action = &ui_action_jump,
            .index = 25
        },
        .render = &ui_render_menu_item,
        .render_data = "Special Fn",
        .render_check = &tick_2hz
    },
    // 25
    {
        .left =
        {
            .action = &ui_render_configure_module_special_function_change,
            .value_direction = false
        },
        .right =
        {
            .action = &ui_render_configure_module_special_function_change,
            .value_direction = true
        },
        .press =
        {
            .action = &ui_render_configure_module_special_function_press,
            .alt_index = 24
        },
        .render = &ui_render_configure_module_special_function,
        .render_check = &tick_2hz
    },
    // 26
    {
        .right =
        {
            .action = &ui_action_jump,
            .index = 27
        },
        .press =
        {
            .action = &ui_action_jump,
            .index = 0
        },
        .render = &ui_render_menu_item,
        .render_data = "Back",
        .render_check = &tick_2hz
    },
    // 27
    {
        .left =
        {
            .action = &ui_action_jump,
            .index = 26
        },
        .press =
        {
            .action = &ui_game_quick_start,
            .index = 28
        },
        .right =
        {
            .action = &ui_action_jump,
            .index = 21
        },
        .render = &ui_render_menu_item,
        .render_data = "Quick Start",
        .render_check = &tick_2hz
    },
    // 28
    {
        .left =
        {
            .action = &ui_game_edgework_change,
            .value_direction = false
        },
        .right =
        {
            .action = &ui_game_edgework_change,
            .value_direction = true
        },
        .press =
        {
            .action = &ui_game_edgework_press,
            .index = 29
        },
        .render = &ui_game_edgework_display,
        .render_check = &tick_2hz
    },
    // 29
    {
        .right =
        {
            .action = &ui_action_jump,
            .index = 30
        },
        .press =
        {
            .action = &ui_action_jump,
            .index = 28
        },
        .render = &ui_render_menu_item,
        .render_data = "Edgework",
        .render_check = &tick_2hz
    },
    // 30
    {
        .left =
        {
            .action = &ui_action_jump,
            .index = 29
        },
        .right =
        {
            .action = &ui_action_jump,
            .index = 31
        },
        .press =
        {
            .action = &ui_game_abandon,
            .index = 0
        },
        .render = &ui_render_menu_item,
        .render_data = "Abandon Game",
        .render_check = &tick_2hz
    },
    // 31
    {
        .left =
        {
            .action = &ui_action_jump,
            .index = 30
        },
        .press =
        {
            .action = &ui_action_jump,
            .index = 33
        },
        .render = &ui_render_menu_item,
        .render_data = "Debug",
        .render_check = &tick_2hz
    },
    // 32
    {
        .right =
        {
            .action = &ui_action_jump,
            .index = 33
        },
        .press =
        {
            .action = &ui_action_jump,
            .index = 31
        },
        .render = &ui_render_menu_item,
        .render_data = "Back",
        .render_check = &tick_2hz
    },
    // 33
    {
        .left =
        {
            .action = &ui_action_jump,
            .index = 32
        },
        .press =
        {
            .action = &ui_game_force_strike,
        },
        .render = &ui_render_menu_item,
        .render_data = "Force Strike",
        .render_check = &tick_2hz
    },
    // 34
    {
        .press =
        {
            .action = &ui_game_idle,
            .index = 0
        },
        .render = &ui_game_endgame_display,
        .render_check = &tick_2hz
    },
    // 35
    {
        .right =
        {
            .action = &ui_action_jump,
            .index = 36
        },
        .press =
        {
            .action = &ui_action_jump,
            .index = 5
        },
        .render = &ui_render_menu_item,
        .render_data = "Back",
        .render_check = &tick_2hz
    },
    // 36
    {
        .left =
        {
            .action = &ui_action_jump,
            .index = 35
        },
        .press =
        {
            .action = &ui_render_configure_global_argb_brightness_initial,
            .index = 38
        },
        .right =
        {
            .action = &ui_action_jump,
            .index = 37
        },
        .render = &ui_render_menu_item,
        .render_data = "ARGB LED Bri",
        .render_check = &tick_2hz
    },
    // 37
    {
        .left =
        {
            .action = &ui_action_jump,
            .index = 36,
        },
        .press =
        {
            .action = &ui_render_configure_global_buzzer_vol_initial,
            .index = 39
        },
        .render = &ui_render_menu_item,
        .render_data = "Buzzer Vol",
        .render_check = &tick_2hz
    },
    // 38
    {
        .left =
        {
            .action = &ui_render_configure_global_argb_brightness_change,
            .value_direction = false
        },
        .right =
        {
            .action = &ui_render_configure_global_argb_brightness_change,
            .value_direction = true
        },
        .press =
        {
            .action = &ui_render_configure_global_argb_brightness_press,
            .alt_index = 36
        },
        .render = &ui_render_configure_global_argb_brightness,
        .render_check = &tick_2hz
    },
    // 39
    {
        .left =
        {
            .action = &ui_render_configure_global_buzzer_vol_change,
            .value_direction = false
        },
        .right =
        {
            .action = &ui_render_configure_global_buzzer_vol_change,
            .value_direction = true
        },
        .press =
        {
            .action = &ui_render_configure_global_buzzer_vol_press,
            .alt_index = 37
        },
        .render = &ui_render_configure_global_buzzer_vol,
        .render_check = &tick_2hz
    },
    // 40
    {
        .left =
        {
            .action = &ui_game_custom_module_count_change,
            .value_direction = false
        },
        .right =
        {
            .action = &ui_game_custom_module_count_change,
            .value_direction = true
        },
        .press =
        {
            .action = &ui_game_custom_module_count_press,
            .index = 41,
            .alt_index = 21
        },
        .render = &ui_game_custom_module_count_display,
        .render_check = &tick_2hz
    },
    // 41
    {
        .left =
        {
            .action = &ui_game_custom_time_change,
            .value_direction = false
        },
        .right =
        {
            .action = &ui_game_custom_time_change,
            .value_direction = true
        },
        .press =
        {
            .action = &ui_game_custom_time_press,
            .index = 42,
            .alt_index = 40
        },
        .render = &ui_game_custom_time_display,
        .render_check = &tick_2hz
    },
    // 42
    {
        .left =
        {
            .action = &ui_game_custom_strikes_change,
            .value_direction = false
        },
        .right =
        {
            .action = &ui_game_custom_strikes_change,
            .value_direction = true
        },
        .press =
        {
            .action = &ui_game_custom_strikes_press,
            .index = 43,
            .alt_index = 41
        },
        .render = &ui_game_custom_strikes_display,
        .render_check = &tick_2hz
    },
    // 43
    {
        .press =
        {
            .action = &ui_game_custom_start_press,
            .index = 28
        },
        .render = &ui_game_custom_start_display,
        .render_check = &tick_2hz
    },
    // 44
    {
        .left =
        {
            .action = &ui_render_configure_module_opt_port_change,
            .value_direction = false
        },
        .right =
        {
            .action = &ui_render_configure_module_opt_port_change,
            .value_direction = true
        },
        .press =
        {
            .action = &ui_render_configure_module_opt_port_press,
            .index = 45,
            .alt_index = 17
        },
        .render = &ui_render_configure_module_opt_port,
        .render_check = &tick_2hz
    },
    // 45
    {
        .left =
        {
            .action = &ui_render_configure_module_opt_set_change,
            .value_direction = false
        },
        .right =
        {
            .action = &ui_render_configure_module_opt_set_change,
            .value_direction = true
        },
        .press =
        {
            .action = &ui_render_configure_module_opt_set_press,
            .alt_index = 44
        },
        .render = &ui_render_configure_module_opt_set,
        .render_check = &tick_2hz
    },
};

uint8_t current = UI_IDX_ROOT;

void ui_initialise(void) {
    /* Initialise the rotary encoder. */
    rotary_initialise(KPIN_B1, KPIN_B0);
    /* Initialise keymatrix. */
    keymatrix_initialise(&ui_cols[0], &ui_rows[0], KEYMODE_COL_ONLY);

    /* Initial UI Sync. */
    ui_force(current);
}

void ui_force(uint8_t i) {
    current = i;
    interface[i].render(&interface[i]);
    lcd_sync();
}

void ui_service(void) {
    /* Service rotary encoder. */
    rotary_service();
    /* Service keymatrix. */
    keymatrix_service();

    /* Handle rotary encoder being turned. */
    int8_t rotations = rotary_fetch_delta();
    rotary_clear();

    uint8_t presses = 0;

    /* Handle rotary encoder being pressed. */
    for (uint8_t press = keymatrix_fetch(); press != KEY_NO_PRESS; press = keymatrix_fetch()) {
        if (press & KEY_DOWN_BIT) {
            presses++;
        }
    }

    uint8_t original = current;

    if (presses > 0) {
        if (interface[current].press.action != NULL) {
            current = interface[current].press.action(current, &interface[current].press);
            sound_play(SOUND_ALL_PRESS_IN);
        }
    } else {
        if (rotations < 0) {
            for (int8_t i = rotations; i < 0; i++) {
                if (interface[current].left.action != NULL) {
                    current = interface[current].left.action(current, &interface[current].left);
                }
            }
        } else if (rotations > 0) {
            for (int8_t i = 0; i < rotations; i++) {
                if (interface[current].right.action != NULL) {
                    current = interface[current].right.action(current, &interface[current].right);
                }
            }
        }

        if (rotations != 0) {
            sound_play(SOUND_ALL_ROTARY_CLICK);
        }
    }

    if (original != current || *interface[current].render_check) {
        interface[current].render(&interface[current]);
        lcd_sync();
    }
}

uint8_t ui_action_jump(uint8_t current, action_t *a) {
    return a->index;
}

uint8_t ui_action_restart_ktane(uint8_t current, action_t *a) {
    // Send reset onto network.
    module_send_reset();

    // Never reached.
    return current;
}

const char left_arrow[] = {0b01111111};
const char right_arrow[] = {0b01111110};
const char press_arrow[] = {0b10100101};

void ui_render_menu_item(interface_t *current) {
    lcd_clear();
    ui_render_menu_item_text((char *) current->render_data, current->press.action != NULL, current->left.action != NULL, current->right.action != NULL);
}

void ui_render_menu_item_text(char *text, bool press_icons, bool left_icon, bool right_icon) {
    uint8_t size = 0;
    for (char *s = text; *s != '\0'; s++) {
        size++;
    }

    if (press_icons) {
        lcd_update(0, 1, 1, &press_arrow[0]);
        lcd_update(0, 14, 1, &press_arrow[0]);
    }

    if (left_icon) {
        lcd_update(0, 0, 1, &left_arrow[0]);
    }

    if (right_icon) {
        lcd_update(0, 15, 1, &right_arrow[0]);
    }

    uint8_t mid = 8 - (size / 2);
    lcd_update(0, mid, size, text);
}
