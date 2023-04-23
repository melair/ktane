#include <xc.h>
#include "../../buzzer.h"
#include "../../../common/can.h"
#include "../../peripherals/lcd.h"
#include "../../module.h"
#include "../../mode.h"
#include "../../opts.h"
#include "ui.h"
#include "ui_configure_module.h"

int8_t ui_configure_module_selected = 0;

uint8_t ui_action_configure_module_select_change(uint8_t current, action_t *a) {
    int8_t initial = ui_configure_module_selected;

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
        for (int8_t i = ui_configure_module_selected; i >= 0; i--) {
            if (module_get((uint8_t) i) != NULL) {
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
    bool has_right = (module_get((uint8_t) (ui_configure_module_selected + 1)) != NULL);
    bool has_press = (current->press.action != NULL);

    lcd_clear();
    ui_render_menu_item_text((char *) current->render_data, has_press, has_left, has_right);

    if (ui_configure_module_selected == -1) {
        module_send_identify(0xff);
        lcd_update(1, 6, 4, "Back");
    } else {
        module_t *module = module_get((uint8_t) ui_configure_module_selected);

        module_send_identify(module->id);

        lcd_update(1, 0, 15, "ID: -- Mode: --");
        lcd_hex(1, 4, module->id);
        lcd_hex(1, 13, module->mode);
    }
}

int8_t ui_configure_module_hardware = 0;
#define HARDWARE_STATS (3 - 1)

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

    char *title;

    lcd_clear();

    if (ui_configure_module_hardware == -1) {
        title = "Back";
    } else {
        module_t *module = module_get((uint8_t) ui_configure_module_selected);

        switch (ui_configure_module_hardware) {
            case 0:
                title = "Firmware";
                lcd_hex(1, 1, (module->firmware.bootloader >> 8) & 0xff);
                lcd_hex(1, 3, (module->firmware.bootloader) & 0xff);
                lcd_update(1, 5, 1, "/");
                lcd_hex(1, 6, (module->firmware.application >> 8) & 0xff);
                lcd_hex(1, 8, (module->firmware.application) & 0xff);
                lcd_update(1, 10, 1, "/");
                lcd_hex(1, 11, (module->firmware.flasher >> 8) & 0xff);
                lcd_hex(1, 13, (module->firmware.flasher) & 0xff);
                break;
            case 1:
                title = "Opts";
                lcd_update(1, 1, 1, "A:");
                lcd_number(1, 3, 2, opts_get(0));
                lcd_update(1, 6, 1, "B:");
                lcd_number(1, 8, 2, opts_get(1));
                lcd_update(1, 11, 1, "C:");
                lcd_number(1, 13, 2, opts_get(2));
                break;
            case 2:
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
        ui_render_menu_item_text("Back", has_press, has_left, has_right);
    } else {
        module_error_t *error = module_get_errors((uint8_t) ui_configure_module_selected, (uint8_t) ui_configure_module_errors);
        ui_render_menu_item_text("Error -", has_press, has_left, has_right);

        lcd_number(0, 11, 1, ((uint8_t) ui_configure_module_errors));

        lcd_update(1, 2, 11, "0x---- --- -");

        lcd_hex(1, 4, ((error->code >> 8) & 0xff));
        lcd_hex(1, 6, (error->code & 0xff));

        lcd_number(1, 9, 3, error->count);

        char *active = "N";

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

    char *title;

    lcd_clear();

    if (ui_configure_module_can_stats == -1) {
        title = "Back";
    } else {
        if (ui_configure_module_selected != 0) {
            title = "Unavailable";
            lcd_update(1, 4, 8, "Remotely");
        } else {
            can_statistics_t *stats = can_get_statistics();

            switch (ui_configure_module_can_stats) {
                case 0:
                    title = "Packets";
                    lcd_update(1, 0, 15, "T:-----  R:-----");
                    lcd_number(1, 2, 5, stats->tx_packets);
                    lcd_number(1, 11, 5, stats->rx_packets);
                    break;
                case 1:
                    title = "Errors";
                    lcd_update(1, 0, 15, "T:-----  R:-----");
                    lcd_number(1, 2, 5, stats->tx_errors);
                    lcd_number(1, 11, 5, stats->rx_errors);
                    break;
                case 2:
                    title = "Overflow";
                    lcd_update(1, 0, 15, "T:-----  R:-----");
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
        module_t *module = module_get((uint8_t) ui_configure_module_selected);
        module_send_mode_set(module->id, mode_id_by_index((uint8_t) ui_configure_module_mode_set));
    }

    ui_configure_module_mode_set = 0;
    return a->alt_index;
}

void ui_render_configure_module_mode_set(interface_t *current) {
    bool has_left = (ui_configure_module_mode_set >= 0);
    bool has_right = (ui_configure_module_mode_set < MODES_STATS);
    bool has_press = true;

    lcd_clear();
    char *title;

    if (ui_configure_module_mode_set == -1) {
        title = "Cancel";
    } else {
        title = "Change to";

        char *name = mode_name_by_index((uint8_t) ui_configure_module_mode_set);

        uint8_t size = 0;
        for (char *s = name; *s != '\0' && size < 16; s++) {
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
        module_t *module = module_get((uint8_t) ui_configure_module_selected);
        module_send_special_function(module->id, (ui_configure_module_special_function & 0xff));
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
    char *title;

    if (ui_configure_module_special_function == -1) {
        title = "Back";
    } else {
        title = "Special Fn";

        lcd_hex(1, 7, (uint8_t) ui_configure_module_special_function);
    }

    ui_render_menu_item_text(title, has_press, has_left, has_right);
}

int8_t ui_configure_module_opt_port_id = 0;
#define OPT_SELECT_MAX 3

uint8_t ui_render_configure_module_opt_port_change(uint8_t current, action_t *a) {
    if (a->value_direction) {
        if (ui_configure_module_opt_port_id < OPT_SELECT_MAX) {
            ui_configure_module_opt_port_id++;
            buzzer_on_timed(BUZZER_DEFAULT_VOLUME, BUZZER_DEFAULT_FREQUENCY, 10);
        }
    } else {
        if (ui_configure_module_opt_port_id >= 0) {
            ui_configure_module_opt_port_id--;
            buzzer_on_timed(BUZZER_DEFAULT_VOLUME, BUZZER_DEFAULT_FREQUENCY, 10);
        }
    }

    return current;
}

uint8_t ui_render_configure_module_opt_port_press(uint8_t current, action_t *a) {
    if (ui_configure_module_opt_port_id != -1) {
        return a->index;
    }

    ui_configure_module_opt_port_id = 0;
    return a->alt_index;
}

void ui_render_configure_module_opt_port(interface_t *current) {
    bool has_left = (ui_configure_module_opt_port_id >= 0);
    bool has_right = (ui_configure_module_opt_port_id < OPT_SELECT_MAX);
    bool has_press = true;

    lcd_clear();
    char *title;

    if (ui_configure_module_opt_port_id == -1) {
        title = "Back";
    } else {
        title = "Select";

        lcd_update(1, 5, 4, "Port");

        char c[2];
        c[0] = 'A' + ((uint8_t) ui_configure_module_opt_port_id);
        c[1] = '\0';

        lcd_update(1, 10, 1, &c[0]);
    }

    ui_render_menu_item_text(title, has_press, has_left, has_right);
}

int8_t ui_configure_module_opt_set_id = 0;

uint8_t ui_render_configure_module_opt_set_change(uint8_t current, action_t *a) {
    if (a->value_direction) {
        if (ui_configure_module_opt_set_id < OPT_COUNT) {
            ui_configure_module_opt_set_id++;
            buzzer_on_timed(BUZZER_DEFAULT_VOLUME, BUZZER_DEFAULT_FREQUENCY, 10);
        }
    } else {
        if (ui_configure_module_opt_set_id >= 0) {
            ui_configure_module_opt_set_id--;
            buzzer_on_timed(BUZZER_DEFAULT_VOLUME, BUZZER_DEFAULT_FREQUENCY, 10);
        }
    }

    return current;
}

uint8_t ui_render_configure_module_opt_set_press(uint8_t current, action_t *a) {
    if (ui_configure_module_opt_set_id != -1) {
        module_t *module = module_get((uint8_t) ui_configure_module_selected);
        packet_outgoing.module.set_opt.can_id = module->id;
        packet_outgoing.module.set_opt.port = (uint8_t) ui_configure_module_opt_port_id;
        packet_outgoing.module.set_opt.opt = (uint8_t) ui_configure_module_opt_set_id;
        packet_send(PREFIX_MODULE, OPCODE_MODULE_OPT_SET, SIZE_MODULE_OPT_SET, &packet_outgoing);
    }

    ui_configure_module_opt_set_id = 0;
    return a->alt_index;
}

void ui_render_configure_module_opt_set(interface_t *current) {
    bool has_left = (ui_configure_module_opt_set_id >= 0);
    bool has_right = (ui_configure_module_opt_set_id < OPT_COUNT);
    bool has_press = true;

    lcd_clear();
    char *title;

    if (ui_configure_module_opt_set_id == -1) {
        title = "Back";
    } else {
        title = "Change to";

        const char *name = &opts_name[ui_configure_module_opt_set_id][0];

        uint8_t size = 0;
        for (const char *s = name; *s != '\0' && size < 16; s++) {
            size++;
        }

        uint8_t start = (8 - (size / 2));
        lcd_update(1, start, size, name);
    }

    ui_render_menu_item_text(title, has_press, has_left, has_right);
}
