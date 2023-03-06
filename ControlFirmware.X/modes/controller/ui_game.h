#ifndef UI_GAME_H
#define	UI_GAME_H

uint8_t ui_game_quick_start(uint8_t current, action_t *a);
uint8_t ui_game_abandon(uint8_t current, action_t *a);
uint8_t ui_game_force_strike(uint8_t current, action_t *a);

uint8_t ui_game_edgework_change(uint8_t current, action_t *a);
uint8_t ui_game_edgework_press(uint8_t current, action_t *a);
void ui_game_edgework_display(interface_t *current);

void ui_game_endgame_display(interface_t *current);

#endif	/* UI_GAME_H */
