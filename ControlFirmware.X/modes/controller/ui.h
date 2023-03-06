#ifndef UI_H
#define	UI_H

#include <stdbool.h>

typedef struct action action_t;
typedef struct interface interface_t;

struct action {
    uint8_t(*action)(uint8_t, action_t *);

    union {
        struct {
            uint8_t index;
            uint8_t alt_index;
        };
        bool value_direction;
    };
};

struct interface {
    action_t left;
    action_t right;
    action_t press;

    void (*render)(interface_t *);
    void *render_data;
    bool *render_check;
};

typedef struct {
} mode_controller_ui_t;

void ui_initialise(void);
void ui_service(void);
void ui_render_menu_item_text(uint8_t *text, bool press_icons, bool left_icon, bool right_icon);
void ui_force(uint8_t i);

#define UI_IDX_ROOT     0
#define UI_IDX_GAME_END 34

#endif	/* UI_H */

