#include <xc.h>
#include <stdbool.h>
#include "game.h"
#include "buzzer.h"
#include "tick.h"
#include "module.h"
#include "mode.h"
#include "status.h"
#include "../common/can.h"
#include "../common/packet.h"
#include "edgework.h"

/* Game state. */
game_t game;
module_game_t *this_module;

/* Local function prototypes. */
void game_service_init(void);
void game_service_idle(void);
void game_service_start(void);
void game_service_setup(void);
void game_service_running(void);
void game_service_over(void);
uint8_t game_find_or_create_module(uint8_t id);
void game_module_state_send(void);

/**
 * Initialise the game state.
 */
void game_initialise(void) {
    game.state = GAME_INIT;
    this_module = module_get_game(0);
}

/**
 * Service the game state.
 */
void game_service(void) {
    if (!tick_2khz) {
        return;
    }

    bool is_first = game.state_first;

    switch (game.state) {
        case GAME_INIT:
            game_service_init();
        case GAME_IDLE:
            game_service_idle();
            break;
        case GAME_SETUP:
            game_service_setup();
            break;
        case GAME_START:
            game_service_start();
            break;
        case GAME_RUNNING:
            game_service_running();
            break;
        case GAME_OVER:
            game_service_over();
            break;
    }

    if (is_first) {
        game.state_first = false;
    }
}

/**
 * For use by the controller, create a new game, initialising game basics. Will
 * notify all other modules and move the game to SETUP state.
 *
 * @param seed seed, used to calculate edge work and other variables
 * @param strikes maximum number of strikes
 * @param minutes number of minutes player has to solve game
 * @param seconds number of seconds on top of minutes player has to solve game
 */
void game_create(uint32_t seed, uint8_t strikes_total, uint8_t minutes, uint8_t seconds) {
    /* To save a little firmware space, use the outgoing packet temporarily to
     * allow reuse of game_update. */
    packet_outgoing.opcode = OPCODE_GAME_STATE;
    packet_outgoing.game.state.state = GAME_SETUP;
    packet_outgoing.game.state.seed = seed;
    packet_outgoing.game.state.strikes_current = 0;
    packet_outgoing.game.state.strikes_total = strikes_total;
    packet_outgoing.game.state.minutes = minutes;
    packet_outgoing.game.state.seconds = seconds;
    packet_outgoing.game.state.centiseconds = 0;
    packet_outgoing.game.state.time_ratio = TIME_RATIO_1;

    game_receive_update(0, &packet_outgoing);
    game_update_send();
}

/**
 * For use by the controller, change the state of the game.
 *
 * @param state game state
 * @param result game result
 */
void game_set_state(uint8_t state, uint8_t result) {
    game.state = state;
    game.state_first = true;
    game.result = result;

    game_update_send();
}

/**
 * For use by the controller, publish the games current state to the network.
 */
void game_update_send(void) {
    packet_outgoing.game.state.state = game.state;
    packet_outgoing.game.state.seed = game.seed;
    packet_outgoing.game.state.strikes_current = game.strikes_current;
    packet_outgoing.game.state.strikes_total = game.strikes_total;
    packet_outgoing.game.state.minutes = game.time_remaining.minutes;
    packet_outgoing.game.state.seconds = game.time_remaining.seconds;
    packet_outgoing.game.state.centiseconds = game.time_remaining.centiseconds;
    packet_outgoing.game.state.time_ratio = game.time_ratio;
    packet_send(PREFIX_GAME, OPCODE_GAME_STATE, SIZE_GAME_STATE, &packet_outgoing);
}

/**
 * Called from CAN/protocol, update internal state with game state. Ideally
 * if game is running, broadcast mid second, as any corrections to clock drift
 * will then be invisible to player.
 *
 * @param id can ID of sending node
 * @param p packet sent by node
 */
void game_receive_update(uint8_t id, packet_t *p) {
    if (game.state != p->game.state.state) {
        game.state_first = true;
    }

    if (game.seed != p->game.state.seed) {
        game.seed = p->game.state.seed;
        edgework_generate(game.seed, 255);
        game.module_seed = game.seed ^ (uint32_t) can_get_id();
    }

    game.state = p->game.state.state;
    game.strikes_current = p->game.state.strikes_current;
    game.strikes_total = p->game.state.strikes_total;
    game.time_remaining.minutes = p->game.state.minutes;
    game.time_remaining.seconds = p->game.state.seconds;
    game.time_remaining.centiseconds = p->game.state.centiseconds;
    game.time_remaining.done = (p->game.state.minutes == 0 && p->game.state.seconds == 0 && p->game.state.centiseconds == 0);
    game.time_ratio = p->game.state.time_ratio;
}

/**
 * Called from CAN/protocol, update the number of strikes the game has had.
 *
 * @param id CAN id of sending node
 * @param p inbound can packet
 */
void game_receive_strike_update(uint8_t id, packet_t *p) {
    game.strikes_current += p->game.module_strike.strikes;
}

/**
 * Called from CAN/protocol, update a modules current state.
 *
 * @param id CAN id
 * @param p inbound can packet
 */
void game_receive_module_update_state(uint8_t id, packet_t *p) {
    module_game_t *that_module = module_get_game_by_id(id);

    if (that_module == NULL) {
        return;
    }

    that_module->ready = p->game.module_state.flags.ready;
    that_module->solved = p->game.module_state.flags.solved;
}

/**
 * For use by the controller, configure a module.
 *
 * @param id CAN id
 * @param enabled true if the module is to be enabled
 * @param difficulty difficulty the module should configure its self to be if
 *                   it supports variable difficulty.
 */
void game_module_config_send(uint8_t id, bool enabled, uint8_t difficulty) {
    packet_outgoing.game.module_config.can_id = id;
    packet_outgoing.game.module_config.flags.enabled = enabled;
    packet_outgoing.game.module_config.difficulty = difficulty;
    packet_send(PREFIX_GAME, OPCODE_GAME_MODULE_CONFIG, SIZE_GAME_MODULE_CONFIG, &packet_outgoing);

    game_receive_module_config(0, &packet_outgoing);
}

/**
 *  Called from CAN/protocol, update a modules configuration in store.
 *
 * @param id CAN id
 * @param p inbound packet
 */
void game_receive_module_config(uint8_t id, packet_t *p) {
    module_game_t *that_module = module_get_game_by_id(p->game.module_config.can_id);

    if (that_module == NULL) {
        return;
    }

    that_module->enabled = p->game.module_config.flags.enabled;
    that_module->difficulty = p->game.module_config.difficulty;
}

void game_module_state_send(void) {
    packet_outgoing.game.module_state.flags.ready = this_module->ready;
    packet_outgoing.game.module_state.flags.solved = this_module->solved;
    packet_send(PREFIX_GAME, OPCODE_GAME_MODULE_STATE, SIZE_GAME_MODULE_STATE, &packet_outgoing);
}

/**
 * For use by the mode, announce the module ready state.

 * @param ready true if module is ready
 */
void game_module_ready(bool ready) {
    this_module->ready = ready;
    game_module_state_send();

    if (ready) {
        status_set(STATUS_READY);
    } else {
        status_set(STATUS_SETUP);
    }
}

/**
 * For use by the mode, announce the module solved state.

 * @param solved true if module is ready
 */
void game_module_solved(bool solved) {
    this_module->solved = solved;
    game_module_state_send();

    if (solved) {
        status_set(STATUS_SOLVED);
    } else {
        status_set(STATUS_UNSOLVED);
    }
}

/**
 * For use by the mode, announce the module has caused a strike.

 * @param strikes the number of strikes to add to game
 */
void game_module_strike(uint8_t strikes) {
    packet_outgoing.game.module_strike.strikes = strikes;
    packet_send(PREFIX_GAME, OPCODE_GAME_MODULE_STRIKE, SIZE_GAME_MODULE_STRIKE, &packet_outgoing);

    buzzer_on_timed(BUZZER_DEFAULT_VOLUME, BUZZER_DEFAULT_FREQUENCY, 750);
}

/**
 * Service a game that is initing.
 */
void game_service_init(void) {
    if (can_ready()) {
        game_set_state(GAME_IDLE, RESULT_NONE);
    }
}

/**
 * Service a game being idle.
 */
void game_service_idle(void) {
    if (tick_20hz) {
        /* Set status led to ready. */
        status_set(STATUS_IDLE);
    }

    if (game.state_first) {
        for (uint8_t i = 0; i < MODULE_COUNT; i++) {
            module_game_t *that_module = module_get_game(i);

            if (that_module == NULL) {
                break;
            }

            that_module->enabled = false;
            that_module->ready = false;
            that_module->solved = false;
        }
    }
}

/**
 * Service a game being setup, on first run clear database to ensure modules
 * are reset to blank.
 */
void game_service_setup(void) {
    if (tick_20hz) {
        if (this_module->enabled) {
            if (this_module->ready) {
                status_set(STATUS_READY);
            } else {
                status_set(STATUS_SETUP);
            }
        } else {
            status_set(STATUS_UNUSED);
        }
    }
}

/**
 * Service a game being started.
 */
void game_service_start(void) {
    if (tick_20hz) {
        if (this_module->enabled) {
            status_set(STATUS_UNSOLVED);
        } else {
            status_set(STATUS_UNUSED);
        }
    }
}

/* Number of the Hz of the 2kHz ticker to count towards a centisecond on the clock. */
const uint8_t quantum_per_ms_ratio[5] = {
    20, // Target 1.0s   Actual 1.0s   Error: 0%
    16, // Target 0.8s   Actual 0.8s   Error: 0%
    13, // Target 0.67s  Actual 0.65s  Error: 3%
    11, // Target 0.57s  Actual 0.55s  Error: 3.5%
    10, // Target 0.5s   Actual 0.5s   Error: 0%
};

/* Current quantum. */
uint8_t quantum;

/**
 * Service the game running, primarily used to maintain internal clock.
 */
void game_service_running(void) {
    if (tick_20hz) {
        if (this_module->enabled) {
            if (this_module->solved) {
                status_set(STATUS_SOLVED);

                if (this_module->solved_tick < 15) {
                    switch (this_module->solved_tick) {
                        case 0:
                            buzzer_on_timed(BUZZER_DEFAULT_VOLUME, BUZZER_FREQ_D5_SHARP, 700);
                            break;
                        case 14:
                            buzzer_on_timed(BUZZER_DEFAULT_VOLUME, BUZZER_FREQ_F5_SHARP, 700);
                            break;
                    }
                    this_module->solved_tick++;
                }
            } else {
                status_set(STATUS_UNSOLVED);
            }
        } else {
            status_set(STATUS_UNUSED);
        }
    }

    if (quantum_per_ms_ratio[game.time_ratio] == quantum) {
        if (!game.time_remaining.done) {
            if (game.time_remaining.centiseconds <= 0) {
                game.time_remaining.centiseconds = 99;

                if (game.time_remaining.seconds == 0) {
                    game.time_remaining.seconds = 59;

                    if (game.time_remaining.minutes == 0) {
                        game.time_remaining.done = true;
                        game.time_remaining.centiseconds = 0;
                        game.time_remaining.seconds = 0;
                    } else {
                        game.time_remaining.minutes--;
                    }
                } else {
                    game.time_remaining.seconds--;
                }
            } else {
                game.time_remaining.centiseconds--;
            }
        }
        quantum = 0;
    } else {
        quantum++;
    }
}

/* Define notes to play on success. */
const uint16_t success_notes[] = {BUZZER_FREQ_G5, BUZZER_FREQ_F5_SHARP, BUZZER_FREQ_D5_SHARP, BUZZER_FREQ_A4, BUZZER_FREQ_G4_SHARP, BUZZER_FREQ_E5, BUZZER_FREQ_G5_SHARP, BUZZER_FREQ_C6, 0x0000};

/* Duration notes should play for. */
#define NOTE_DURATION 250
/* Interval notes should play at. */
#define NOTE_INTERVAL 300

/* Late time a note was played. */
uint32_t note_last_play = 0;
/* Last note played. */
uint8_t note = 0;

/**
 *  Service a game being over.
 */
void game_service_over(void) {
    if (game.state_first) {
        note_last_play = 0;
        note = 0;

        if (game.result == RESULT_FAILURE) {
            buzzer_on_timed(BUZZER_DEFAULT_VOLUME, BUZZER_DEFAULT_FREQUENCY, 1000);
        }
    }

    if (tick_20hz) {
        if (game.result == RESULT_SUCCESS) {
            if (tick_value - note_last_play > NOTE_INTERVAL && success_notes[note] != 0x000) {
                note_last_play = tick_value;

                buzzer_on_timed(BUZZER_DEFAULT_VOLUME, success_notes[note], NOTE_DURATION);
                note++;
            }
        }
    }
}