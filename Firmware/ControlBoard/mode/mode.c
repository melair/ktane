#include "mode.h"
#include "mode_definitions.h"
#include "../config.h"
#include <hal/nvm.h>
#include <utils/fsm.h>
#include <stdint.h>
#include <xc.h>

static fs_t state_init;
static fs_t state_startup;
static fs_t state_idle;
static fs_t state_attract;
static fs_t state_prepare;
static fs_t state_ready;
static fs_t state_starting;
static fs_t state_running;
static fs_t state_solved;
static fs_t state_abort;
static fs_t state_over;

static fs_t state_init = {.id = MODE_STATE_INIT, .next_states = {&state_startup, NULL}};
static fs_t state_startup = {.id = MODE_STATE_STARTUP, .next_states = {&state_idle, NULL}};
static fs_t state_idle = {.id = MODE_STATE_IDLE, .next_states = {&state_attract, &state_prepare, NULL}};
static fs_t state_attract = {.id = MODE_STATE_ATTRACT, .next_states = {&state_idle, NULL}};
static fs_t state_prepare = {.id = MODE_STATE_PREPARE, .next_states = {&state_ready, &state_abort, NULL}};
static fs_t state_ready = {.id = MODE_STATE_READY, .next_states = {&state_starting, &state_abort, NULL}};
static fs_t state_starting = {.id = MODE_STATE_STARTING, .next_states = {&state_running, &state_abort, NULL}};
static fs_t state_running = {.id = MODE_STATE_RUNNING, .next_states = {&state_over, &state_abort, NULL}};
static fs_t state_over = {.id = MODE_STATE_OVER, .next_states = {&state_idle, NULL}};
static fs_t state_abort = {.id = MODE_STATE_ABORT, .next_states = {&state_idle, NULL}};

static fs_t *states[MODE_STATE_COUNT] = { &state_init, &state_startup, &state_idle, &state_attract, &state_prepare, &state_ready, &state_starting, &state_running, &state_over, &state_abort};
static fsm_t mode_fsm;

static const mode_t *current_mode;

void mode_init(void) {
  uint8_t selected;

  /* Load current mode. */
  nvm_eeprom_read(CONFIG_LOC_MODE, &selected, sizeof(uint8_t));

  /* Find the mode, but fallback to blank. */
  current_mode = &mode_definitions[MODE_BLANK];

  for (uint8_t i = 0; i < MODE_COUNT; i++) {
    if (mode_definitions[i].external_id == selected) {
      current_mode = &mode_definitions[i];
      break;
    }
  }

  for (uint8_t i = 0; i < MODE_STATE_COUNT; i++) {
    states[i]->enter = current_mode->state_funcs[i]->entry;
    states[i]->service = current_mode->state_funcs[i]->service;
    states[i]->exit = current_mode->state_funcs[i]->exit;
  }

  /* Initialise FSM. */
  fsm_init(&mode_fsm, &state_init, NULL);
}

void mode_service(void) {
  fsm_service(&mode_fsm);
}
