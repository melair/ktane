#include <xc.h>
#include "ui.h"
#include "ui_configure_module.h"
#include "../../peripherals/rotary.h"
#include "../../peripherals/keymatrix.h"
#include "../../buzzer.h"
#include "../../peripherals/lcd.h"
#include "../../tick.h"
#include "../../protocol_module.h"

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
        .render = &ui_render_menu_item,
        .render_data = "Modules",
        .render_check = &tick_2hz
    },
    // 5 (UNUSED)
    {},
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
        .press = {
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
        .render = &ui_render_menu_item,
        .render_data = "ARGB LED Bri",
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
            .index = 21
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
            .index = 20
        },
        .right =
        {
            .action = &ui_action_jump,
            .index = 22
        },
        .render = &ui_render_menu_item,
        .render_data = "Buzzer Vol",
        .render_check = &tick_2hz
    },
    // 22
    {
        .left =
        {
            .action = &ui_action_jump,
            .index = 21
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
};

uint8_t current = 0;

void ui_initialise(void) {
    /* Initialise the rotary encoder. */
    rotary_initialise(KPIN_B1, KPIN_B0);
    /* Initialise keymatrix. */
    keymatrix_initialise(&ui_cols[0], &ui_rows[0], KEYMODE_COL_ONLY);

    /* Initial UI Sync. */
    interface[current].render(&interface[current]);
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
    }

    if (original != current) {
        buzzer_on_timed(BUZZER_DEFAULT_VOLUME, BUZZER_DEFAULT_FREQUENCY, 10);
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
    protocol_module_reset_send();
    
    // Never reached.
    return current;
}

const uint8_t *left_arrow = {0b01111111, 0};
const uint8_t *right_arrow = {0b01111110, 0};
const uint8_t *press_arrow = {0b10100101, 0};

void ui_render_menu_item(interface_t *current) {
    lcd_clear();
    ui_render_menu_item_text((uint8_t *) current->render_data, current->press.action != NULL, current->left.action != NULL, current->right.action != NULL);
}

void ui_render_menu_item_text(uint8_t *text, bool press_icons, bool left_icon, bool right_icon) {
    uint8_t size = 0;
    for (uint8_t *s = text; *s != '\0'; *s++) {
        size++;
    }

    if (press_icons) {
        lcd_update(0, 1, 1, &press_arrow);
        lcd_update(0, 14, 1, &press_arrow);
    }

    if (left_icon) {
        lcd_update(0, 0, 1, &left_arrow);
    }

    if (right_icon) {
        lcd_update(0, 15, 1, &right_arrow);
    }

    uint8_t mid = 8 - (size / 2);
    lcd_update(0, mid, size, text);
}

/* Game end screen.


        lcd_clear();

        uint8_t *text_strikes = "Strikes: X of X";
        uint8_t *text_remaining = "Time: X:XX.XX";

        lcd_update(0, 0, 15, text_strikes);
        lcd_number(0, 9, 1, game.strikes_current);
        lcd_number(0, 14, 1, game.strikes_total);

        lcd_update(1, 0, 13, text_remaining);
        lcd_number(1, 6, 1, game.time_remaining.minutes);
        lcd_number(1, 8, 2, game.time_remaining.seconds);
        lcd_number(1, 11, 2, game.time_remaining.centiseconds);

        lcd_sync();

 */