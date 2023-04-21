#ifndef UI_CONFIGURE_MODULE_H
#define	UI_CONFIGURE_MODULE_H

void ui_render_configure_module_select(interface_t *current);
uint8_t ui_action_configure_module_select_change(uint8_t current, action_t *a);
uint8_t ui_action_configure_module_select_press(uint8_t current, action_t *a);

uint8_t ui_render_configure_module_hardware_change(uint8_t current, action_t *a);
uint8_t ui_render_configure_module_hardware_press(uint8_t current, action_t *a);
void ui_render_configure_module_hardware(interface_t *current);

uint8_t ui_render_configure_module_errors_change(uint8_t current, action_t *a);
uint8_t ui_render_configure_module_errors_press(uint8_t current, action_t *a);
void ui_render_configure_module_errors(interface_t *current);

uint8_t ui_render_configure_module_can_stats_change(uint8_t current, action_t *a);
uint8_t ui_render_configure_module_can_stats_press(uint8_t current, action_t *a);
void ui_render_configure_module_can_stats(interface_t *current);

uint8_t ui_render_configure_module_mode_set_change(uint8_t current, action_t *a);
uint8_t ui_render_configure_module_mode_set_press(uint8_t current, action_t *a);
void ui_render_configure_module_mode_set(interface_t *current);

uint8_t ui_render_configure_module_special_function_change(uint8_t current, action_t *a);
uint8_t ui_render_configure_module_special_function_press(uint8_t current, action_t *a);
void ui_render_configure_module_special_function(interface_t *current);

uint8_t ui_render_configure_module_opt_port_change(uint8_t current, action_t *a);
uint8_t ui_render_configure_module_opt_port_press(uint8_t current, action_t *a);
void ui_render_configure_module_opt_port(interface_t *current);

uint8_t ui_render_configure_module_opt_set_change(uint8_t current, action_t *a);
uint8_t ui_render_configure_module_opt_set_press(uint8_t current, action_t *a);
void ui_render_configure_module_opt_set(interface_t *current);

#endif	/* UI_CONFIGURE_MODULE_H */

