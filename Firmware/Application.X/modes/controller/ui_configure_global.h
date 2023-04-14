#ifndef UI_CONFIGURE_GLOBAL_H
#define	UI_CONFIGURE_GLOBAL_H

uint8_t ui_render_configure_global_argb_brightness_initial(uint8_t current, action_t *a);
uint8_t ui_render_configure_global_argb_brightness_change(uint8_t current, action_t *a);
uint8_t ui_render_configure_global_argb_brightness_press(uint8_t current, action_t *a);
void ui_render_configure_global_argb_brightness(interface_t *current);

uint8_t ui_render_configure_global_buzzer_vol_initial(uint8_t current, action_t *a);
uint8_t ui_render_configure_global_buzzer_vol_change(uint8_t current, action_t *a);
uint8_t ui_render_configure_global_buzzer_vol_press(uint8_t current, action_t *a);
void ui_render_configure_global_buzzer_vol(interface_t *current);

#endif	/* UI_CONFIGURE_GLOBAL_H */

