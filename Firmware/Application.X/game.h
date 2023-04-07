#ifndef GAME_H
#define	GAME_H

#include <stdbool.h>

void game_initialise(void);
void game_service(void);

void game_create(uint32_t seed, uint8_t strikes, uint8_t minutes, uint8_t seconds);
void game_update_send(void);
void game_set_state(uint8_t state, uint8_t result);

void game_module_config_send(uint8_t id, bool enabled, uint8_t difficulty);

void game_module_ready(bool ready);
void game_module_solved(bool solved);
void game_module_strike(uint8_t strikes);

#define GAME_STATE_COUNT 6

#define GAME_INIT       0
#define GAME_IDLE       1
#define GAME_SETUP      2
#define GAME_START      3
#define GAME_RUNNING    4
#define GAME_OVER       5

#define TIME_RATIO_1        0
#define TIME_RATIO_1_25     1
#define TIME_RATIO_1_5      2
#define TIME_RATIO_1_75     3
#define TIME_RATIO_2        4

#define RESULT_NONE         0
#define RESULT_FAILURE      1
#define RESULT_SUCCESS      2

typedef struct {
    uint32_t seed;
    uint32_t module_seed;

    uint8_t strikes_current;
    uint8_t strikes_total;

    uint8_t state;
    bool state_first;

    uint8_t result;

    struct {
        uint8_t minutes;
        uint8_t seconds;
        uint8_t centiseconds;
        bool done;
    } time_remaining;

    uint8_t time_ratio;
} game_t;

extern game_t game;

typedef struct {
    uint8_t id;
    uint8_t difficulty;
    struct {
        unsigned puzzle      :1;
        unsigned needy       :1;
        unsigned enabled     :1;
        unsigned ready       :1;
        unsigned solved      :1;
        unsigned solved_tick :4;
    };
} module_game_t;

extern module_game_t *this_module;

#endif	/* GAME_H */
