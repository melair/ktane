#include <xc.h>
#include "../../buzzer.h"
#include "../../can.h"
#include "../../peripherals/lcd.h"
#include "../../protocol_module.h"
#include "../../module.h"
#include "../../mode.h"
#include "ui.h"
#include "ui_configure_module.h"

int8_t ui_configure_module_selected = 0;

uint8_t ui_action_configure_module_select_change(uint8_t current, action_t *a) {
    uint8_t initial = ui_configure_module_selected;

    if (a->value_direction) {
        ui_configure_module_selected++;
        buzzer_on_timed(BUZZER_DEFAULT_VOLUME, BUZZER_DEFAULT_FREQUENCY, 10);
    } else {
        if (ui_configure_module_selected >= 0) {
            ui_configure_module_selected--;
            buzzer_on_timed(BUZZER_DEFAULT_VOLUME, BUZZER_DEFAULT_FREQUENCY, 10);
        }
    }

    if (initial != ui_configure_module_selected && ui_configure_module_selected >= 0) {
        for (uint8_t i = ui_configure_module_selected; i >= 0; i--) {
            if (module_get(i) != NULL) {
                ui_configure_module_selected = i;
                break;
            }
        }
    }

    return current;
}

uint8_t ui_action_configure_module_select_press(uint8_t current, action_t *a) {
    if (ui_configure_module_selected == -1) {
        ui_configure_module_selected = 0;        
        return a->alt_index;
    } else {
        return a->index;
    }
}

void ui_render_configure_module_select(interface_t *current) {
    bool has_left = (ui_configure_module_selected >= 0);
    bool has_right = (module_get(ui_configure_module_selected + 1) != NULL);
    bool has_press = (current->press.action != NULL);

    lcd_clear();
    ui_render_menu_item_text((uint8_t *) current->render_data, has_press, has_left, has_right);

    if (ui_configure_module_selected == -1) {
        protocol_module_identify_send(0xff);

        uint8_t *template = "Back";
        lcd_update(1, 6, 4, template);
    } else {
        module_t *module = module_get(ui_configure_module_selected);

        protocol_module_identify_send(module->id);

        uint8_t *template = "ID: -- Mode: --";
        lcd_update(1, 0, 15, template);
        lcd_hex(1, 4, module->id);
        lcd_hex(1, 13, module->mode);
    }
}

int8_t ui_configure_module_hardware = 0;
#define HARDWARE_STATS (2 - 1)

uint8_t ui_render_configure_module_hardware_change(uint8_t current, action_t *a) {
    if (a->value_direction) {
        if (ui_configure_module_hardware < HARDWARE_STATS) {
            ui_configure_module_hardware++;
            buzzer_on_timed(BUZZER_DEFAULT_VOLUME, BUZZER_DEFAULT_FREQUENCY, 10);
        }
    } else {
        if (ui_configure_module_hardware >= 0) {
            ui_configure_module_hardware--;
            buzzer_on_timed(BUZZER_DEFAULT_VOLUME, BUZZER_DEFAULT_FREQUENCY, 10);
        }
    }

    return current;
}

uint8_t ui_render_configure_module_hardware_press(uint8_t current, action_t *a) {
    if (ui_configure_module_hardware == -1) {
        ui_configure_module_hardware = 0;
        return a->alt_index;
    } else {
        return current;
    }
}

void ui_render_configure_module_hardware(interface_t *current) {
    bool has_left = (ui_configure_module_hardware >= 0);
    bool has_right = (ui_configure_module_hardware < HARDWARE_STATS);
    bool has_press = (ui_configure_module_hardware == -1);

    uint8_t *title;

    lcd_clear();

    if (ui_configure_module_hardware == -1) {
        title = "Back";
    } else {
        module_t *module = module_get(ui_configure_module_selected);

        switch(ui_configure_module_hardware) {
            case 0:
                title = "Firmware";
                lcd_hex(1, 6, (module->firmware >> 8) & 0xff);
                lcd_hex(1, 8, (module->firmware) & 0xff);
                break;
            case 1:
                title = "Serial Number";
                lcd_hex(1, 4, (module->serial >> 24) & 0xff);
                lcd_hex(1, 6, (module->serial >> 16) & 0xff);
                lcd_hex(1, 8, (module->serial >> 8) & 0xff);
                lcd_hex(1, 10, (module->serial) & 0xff);
                break;
        }
    }

    ui_render_menu_item_text(title, has_press, has_left, has_right);
}

int8_t ui_configure_module_errors = 0;
#define ERRORS_STATS (ERROR_COUNT - 1)

uint8_t ui_render_configure_module_errors_change(uint8_t current, action_t *a) {
    if (a->value_direction) {
        if (ui_configure_module_errors < ERRORS_STATS) {
            ui_configure_module_errors++;
            buzzer_on_timed(BUZZER_DEFAULT_VOLUME, BUZZER_DEFAULT_FREQUENCY, 10);
        }
    } else {
        if (ui_configure_module_errors >= 0) {
            ui_configure_module_errors--;
            buzzer_on_timed(BUZZER_DEFAULT_VOLUME, BUZZER_DEFAULT_FREQUENCY, 10);
        }
    }

    return current;
}

uint8_t ui_render_configure_module_errors_press(uint8_t current, action_t *a) {
    if (ui_configure_module_errors == -1) {
        ui_configure_module_errors = 0;
        return a->alt_index;
    } else {
        return current;
    }
}

void ui_render_configure_module_errors(interface_t *current) {
    bool has_left = (ui_configure_module_errors >= 0);
    bool has_right = (ui_configure_module_errors < ERRORS_STATS);
    bool has_press = (ui_configure_module_errors == -1);

    lcd_clear();

    if (ui_configure_module_errors == -1) {
        uint8_t *title = "Back";
        ui_render_menu_item_text(title, has_press, has_left, has_right);
    } else {
        module_error_t *error = module_get_errors(ui_configure_module_selected, ui_configure_module_errors);
        uint8_t *title = "Error -";
        ui_render_menu_item_text(title, has_press, has_left, has_right);

        lcd_number(0, 11, 1, ui_configure_module_errors);

        uint8_t *template = "0x---- --- -";
        lcd_update(1, 2, 11, template);

        lcd_hex(1, 4, ((error->code >> 8) & 0xff));
        lcd_hex(1, 6, (error->code & 0xff));

        lcd_number(1, 9, 3, error->count);

        uint8_t *active = "N";

        if (error->active) {
            active = "Y";
        }

        lcd_update(1, 13, 1, active);
    }
}

int8_t ui_configure_module_can_stats = 0;
#define CANS_STATS (5 - 1)

uint8_t ui_render_configure_module_can_stats_change(uint8_t current, action_t *a) {
    if (a->value_direction) {
        if (ui_configure_module_can_stats < CANS_STATS) {
            ui_configure_module_can_stats++;
            buzzer_on_timed(BUZZER_DEFAULT_VOLUME, BUZZER_DEFAULT_FREQUENCY, 10);
        }
    } else {
        if (ui_configure_module_can_stats >= 0) {
            ui_configure_module_can_stats--;
            buzzer_on_timed(BUZZER_DEFAULT_VOLUME, BUZZER_DEFAULT_FREQUENCY, 10);
        }
    }

    return current;
}

uint8_t ui_render_configure_module_can_stats_press(uint8_t current, action_t *a) {
    if (ui_configure_module_can_stats == -1) {
        ui_configure_module_can_stats = 0;
        return a->alt_index;
    } else {
        return current;
    }
}

void ui_render_configure_module_can_stats(interface_t *current) {
    bool has_left = (ui_configure_module_can_stats >= 0);
    bool has_right = (ui_configure_module_can_stats < CANS_STATS);
    bool has_press = (ui_configure_module_can_stats == -1);

    uint8_t *title;

    lcd_clear();

    if (ui_configure_module_can_stats == -1) {
        title = "Back";
    } else {
        if (ui_configure_module_selected != 0) {
            title = "Unavailable";
            uint8_t *remotely = "Remotely";
            lcd_update(1, 4, 8, remotely);
        } else {
            can_statistics_t *stats = can_get_statistics();

            uint8_t *txrx_template = "T:-----  R:-----";

            switch(ui_configure_module_can_stats) {
                case 0:
                    title = "Packets";
                    lcd_update(1, 0, 15, txrx_template);
                    lcd_number(1, 2, 5, stats->tx_packets);
                    lcd_number(1, 11, 5, stats->rx_packets);
                    break;
                case 1:
                    title = "Errors";
                    lcd_update(1, 0, 15, txrx_template);
                    lcd_number(1, 2, 5, stats->tx_errors);
                    lcd_number(1, 11, 5, stats->rx_errors);
                    break;
                case 2:
                    title = "Overflow";
                    lcd_update(1, 0, 15, txrx_template);
                    lcd_number(1, 2, 5, stats->tx_overflow);
                    lcd_number(1, 11, 5, stats->rx_overflow);
                    break;
                case 3:
                    title = "!Rdy / Error";
                    lcd_number(1, 2, 5, stats->tx_not_ready);
                    lcd_number(1, 9, 5, stats->can_error);
                    break;
                case 4:
                    title = "ID Cycles";
                    lcd_number(1, 5, 5, stats->id_cycles);
                    break;
            }
        }
    }

    ui_render_menu_item_text(title, has_press, has_left, has_right);
}

int8_t ui_configure_module_mode_set = 0;
#define MODES_STATS (MODE_COUNT)

uint8_t ui_render_configure_module_mode_set_change(uint8_t current, action_t *a) {
    if (a->value_direction) {
        if (ui_configure_module_mode_set < MODES_STATS) {
            ui_configure_module_mode_set++;
            buzzer_on_timed(BUZZER_DEFAULT_VOLUME, BUZZER_DEFAULT_FREQUENCY, 10);
        }
    } else {
        if (ui_configure_module_mode_set >= 0) {
            ui_configure_module_mode_set--;
            buzzer_on_timed(BUZZER_DEFAULT_VOLUME, BUZZER_DEFAULT_FREQUENCY, 10);
        }
    }

    return current;
}

uint8_t ui_render_configure_module_mode_set_press(uint8_t current, action_t *a) {
    if (ui_configure_module_mode_set != -1) {
        module_t *module = module_get(ui_configure_module_selected);
        protocol_module_mode_set_send(module->id, mode_id_by_index(ui_configure_module_mode_set));
    }
    
    ui_configure_module_mode_set = 0;
    return a->alt_index;
}

void ui_render_configure_module_mode_set(interface_t *current) {
    bool has_left = (ui_configure_module_mode_set >= 0);
    bool has_right = (ui_configure_module_mode_set < MODES_STATS);
    bool has_press = true;

    lcd_clear();
    uint8_t *title;
    
    if (ui_configure_module_mode_set == -1) {
        title = "Cancel";
    } else {
        title = "Change to";

        uint8_t *name = mode_name_by_index(ui_configure_module_mode_set);
        
        uint8_t size = 0;
        for (uint8_t *s = name; *s != '\0' && size < 16; *s++) {
            size++;
        }
        
        uint8_t start = (8 - (size / 2));        
        lcd_update(1, start, size, name);
    }
    
    ui_render_menu_item_text(title, has_press, has_left, has_right);
}

int16_t ui_configure_module_special_function = 0;
#define MAX_SPECIAL_FUNCTION 255

uint8_t ui_render_configure_module_special_function_change(uint8_t current, action_t *a) {
    if (a->value_direction) {
        if (ui_configure_module_special_function < MAX_SPECIAL_FUNCTION) {
            ui_configure_module_special_function++;
            buzzer_on_timed(BUZZER_DEFAULT_VOLUME, BUZZER_DEFAULT_FREQUENCY, 10);
        }
    } else {
        if (ui_configure_module_special_function >= 0) {
            ui_configure_module_special_function--;
            buzzer_on_timed(BUZZER_DEFAULT_VOLUME, BUZZER_DEFAULT_FREQUENCY, 10);
        }
    }

    return current;
}

uint8_t ui_render_configure_module_special_function_press(uint8_t current, action_t *a) {
    if (ui_configure_module_special_function != -1) {
        module_t *module = module_get(ui_configure_module_selected);
        protocol_module_special_function_send(module->id, (ui_configure_module_special_function & 0xff));
        return current;
    } else {    
        ui_configure_module_special_function = 0;
        return a->alt_index;
    }
}

void ui_render_configure_module_special_function(interface_t *current) {
    bool has_left = (ui_configure_module_special_function >= 0);
    bool has_right = (ui_configure_module_special_function < MAX_SPECIAL_FUNCTION);
    bool has_press = true;

    lcd_clear();
    uint8_t *title;
    
    if (ui_configure_module_special_function == -1) {
        title = "Back";
    } else {
        title = "Special Fn";

        lcd_hex(1, 7, ui_configure_module_special_function);
    }
    
    ui_render_menu_item_text(title, has_press, has_left, has_right);
}