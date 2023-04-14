#ifndef UI_GAME_H
#define	UI_GAME_H

uint8_t ui_game_quick_start(uint8_t current, action_t *a);
uint8_t ui_game_abandon(uint8_t current, action_t *a);
uint8_t ui_game_force_strike(uint8_t current, action_t *a);
uint8_t ui_game_idle(uint8_t current, action_t *a);

uint8_t ui_game_edgework_change(uint8_t current, action_t *a);
uint8_t ui_game_edgework_press(uint8_t current, action_t *a);
void ui_game_edgework_display(interface_t *current);

void ui_game_endgame_display(interface_t *current);

uint8_t ui_game_custom_module_count_change(uint8_t current, action_t *a);
uint8_t ui_game_custom_module_count_press(uint8_t current, action_t *a);
void ui_game_custom_module_count_display(interface_t *current);

uint8_t ui_game_custom_time_change(uint8_t current, action_t *a);
uint8_t ui_game_custom_time_press(uint8_t current, action_t *a);
void ui_game_custom_time_display(interface_t *current);

uint8_t ui_game_custom_strikes_change(uint8_t current, action_t *a);
uint8_t ui_game_custom_strikes_press(uint8_t current, action_t *a);
void ui_game_custom_strikes_display(interface_t *current);

uint8_t ui_game_custom_start_press(uint8_t current, action_t *a);
void ui_game_custom_start_display(interface_t *current);

#endif	/* UI_GAME_H */
