#include <xc.h>
#include <stdint.h>
#include "ui.h"
#include "ui_configure_global.h"
#include "../../peripherals/lcd.h"
#include "../../buzzer.h"
#include "../../argb.h"
#include "../../module.h"
#include "../../opts/audio/audio.h"
#include "../../../common/nvm.h"
#include "../../../common/eeprom_addrs.h"

uint8_t ui_configure_global_argb_brightness_initial;
int8_t ui_configure_global_argb_brightness = 0;
#define MAX_ARGB_BRIGHTNESS 31

uint8_t ui_render_configure_global_argb_brightness_initial(uint8_t current, action_t *a) {
    ui_configure_global_argb_brightness_initial = argb_get_brightness();
    ui_configure_global_argb_brightness = ((int8_t) ui_configure_global_argb_brightness_initial);

    return a->index;
}

uint8_t ui_render_configure_global_argb_brightness_change(uint8_t current, action_t *a) {
    if (a->value_direction) {
        if (ui_configure_global_argb_brightness < MAX_ARGB_BRIGHTNESS) {
            ui_configure_global_argb_brightness++;
        }
    } else {
        if (ui_configure_global_argb_brightness > 0) {
            ui_configure_global_argb_brightness--;
        }
    }

    if (ui_configure_global_argb_brightness > 0) {
        argb_set_brightness((uint8_t) ui_configure_global_argb_brightness);
        module_send_global_config(false);
    }

    return current;
}

uint8_t ui_render_configure_global_argb_brightness_press(uint8_t current, action_t *a) {
    if (ui_configure_global_argb_brightness == 0) {
        argb_set_brightness(ui_configure_global_argb_brightness_initial);
        module_send_global_config(false);
    } else {
        module_send_global_config(true);
        nvm_eeprom_write(EEPROM_LOC_ARGB_BRIGHTNESS, (uint8_t) ui_configure_global_argb_brightness);
    }

    return a->alt_index;
}

void ui_render_configure_global_argb_brightness(interface_t *current) {
    bool has_left = (ui_configure_global_argb_brightness > 0);
    bool has_right = (ui_configure_global_argb_brightness < MAX_ARGB_BRIGHTNESS);
    bool has_press = true;

    lcd_clear();
    char *title;

    if (ui_configure_global_argb_brightness == 0) {
        title = "Back";
    } else {
        title = "ARGB LED Bri";

        lcd_number(1, 7, 2, (uint8_t) ui_configure_global_argb_brightness);
    }

    ui_render_menu_item_text(title, has_press, has_left, has_right);
}

uint8_t ui_configure_global_buzzer_vol_initial;
int8_t ui_configure_global_buzzer_vol = 0;
#define MAX_BUZZER_VOLUME 7

uint8_t ui_render_configure_global_buzzer_vol_initial(uint8_t current, action_t *a) {
    ui_configure_global_buzzer_vol_initial = buzzer_get_volume();
    ui_configure_global_buzzer_vol = ((int8_t) ui_configure_global_buzzer_vol_initial);

    return a->index;
}

uint8_t ui_render_configure_global_buzzer_vol_change(uint8_t current, action_t *a) {
    if (a->value_direction) {
        if (ui_configure_global_buzzer_vol < MAX_BUZZER_VOLUME) {
            ui_configure_global_buzzer_vol++;
            buzzer_set_volume((uint8_t) ui_configure_global_buzzer_vol);
            module_send_global_config(false);
        }
    } else {
        if (ui_configure_global_buzzer_vol >= 0) {
            ui_configure_global_buzzer_vol--;
            buzzer_set_volume((uint8_t) ui_configure_global_buzzer_vol);
            module_send_global_config(false);
        }
    }

    return current;
}

uint8_t ui_render_configure_global_buzzer_vol_press(uint8_t current, action_t *a) {
    if (ui_configure_global_buzzer_vol == -1) {
        buzzer_set_volume(ui_configure_global_buzzer_vol_initial);
        module_send_global_config(false);
    } else {
        module_send_global_config(true);
        nvm_eeprom_write(EEPROM_LOC_BUZZER_VOL, (uint8_t) ui_configure_global_buzzer_vol);
    }

    return a->alt_index;
}

void ui_render_configure_global_buzzer_vol(interface_t *current) {
    bool has_left = (ui_configure_global_buzzer_vol >= 0);
    bool has_right = (ui_configure_global_buzzer_vol < MAX_ARGB_BRIGHTNESS);
    bool has_press = true;

    lcd_clear();
    char *title;

    if (ui_configure_global_buzzer_vol == -1) {
        title = "Back";
    } else {
        title = "Buzzer Vol";

        lcd_number(1, 7, 2, (uint8_t) ui_configure_global_buzzer_vol);
    }

    ui_render_menu_item_text(title, has_press, has_left, has_right);
}

uint8_t ui_configure_global_dac_vol_initial;
int8_t ui_configure_global_dac_vol = 0;
#define MAX_DAC_VOLUME 16

uint8_t ui_render_configure_global_dac_vol_initial(uint8_t current, action_t *a) {
    ui_configure_global_dac_vol_initial = audio_get_volume();
    ui_configure_global_dac_vol = ((int8_t) ui_configure_global_dac_vol_initial);

    return a->index;
}

uint8_t ui_render_configure_global_dac_vol_change(uint8_t current, action_t *a) {
    if (a->value_direction) {
        if (ui_configure_global_dac_vol < MAX_DAC_VOLUME) {
            ui_configure_global_dac_vol++;
            audio_set_volume((uint8_t) ui_configure_global_dac_vol);
            module_send_global_config(false);
        }
    } else {
        if (ui_configure_global_dac_vol >= 0) {
            ui_configure_global_dac_vol--;
            audio_set_volume((uint8_t) ui_configure_global_dac_vol);
            module_send_global_config(false);
        }
    }

    return current;
}

uint8_t ui_render_configure_global_dac_vol_press(uint8_t current, action_t *a) {
    if (ui_configure_global_dac_vol == -1) {
        audio_set_volume(ui_configure_global_dac_vol_initial);
        module_send_global_config(false);
    } else {
        module_send_global_config(true);
        nvm_eeprom_write(EEPROM_LOC_DAC_VOL, (uint8_t) ui_configure_global_dac_vol);
    }

    return a->alt_index;
}

void ui_render_configure_global_dac_vol(interface_t *current) {
    bool has_left = (ui_configure_global_dac_vol >= 0);
    bool has_right = (ui_configure_global_dac_vol < MAX_ARGB_BRIGHTNESS);
    bool has_press = true;

    lcd_clear();
    char *title;

    if (ui_configure_global_dac_vol == -1) {
        title = "Back";
    } else {
        title = "DAC Vol";

        lcd_number(1, 7, 2, (uint8_t) ui_configure_global_dac_vol);
    }

    ui_render_menu_item_text(title, has_press, has_left, has_right);
}