#include "mode.h"
#include "mode_definitions.h"
#include "mode_storage.h"
#include "../config.h"
#include <hal/nvm.h>
#include <utils/mem.h>
#include <utils/fsm.h>
#include <stdint.h>
#include <xc.h>

fs_t mode_state_init = {.id = MODE_STATE_INIT, .next_states = {&mode_state_startup, NULL}};
fs_t mode_state_startup = {.id = MODE_STATE_STARTUP, .next_states = {&mode_state_idle, NULL}};
fs_t mode_state_idle = {.id = MODE_STATE_IDLE, .next_states = {&mode_state_attract, &mode_state_prepare, NULL}};
fs_t mode_state_attract = {.id = MODE_STATE_ATTRACT, .next_states = {&mode_state_idle, NULL}};
fs_t mode_state_prepare = {.id = MODE_STATE_PREPARE, .next_states = {&mode_state_ready, &mode_state_abort, NULL}};
fs_t mode_state_ready = {.id = MODE_STATE_READY, .next_states = {&mode_state_starting, &mode_state_abort, NULL}};
fs_t mode_state_starting = {.id = MODE_STATE_STARTING, .next_states = {&mode_state_running, &mode_state_abort, NULL}};
fs_t mode_state_running = {.id = MODE_STATE_RUNNING, .next_states = {&mode_state_over, &mode_state_abort, NULL}};
fs_t mode_state_over = {.id = MODE_STATE_OVER, .next_states = {&mode_state_idle, NULL}};
fs_t mode_state_abort = {.id = MODE_STATE_ABORT, .next_states = {&mode_state_idle, NULL}};

static fs_t *states[MODE_STATE_COUNT] = { &mode_state_init, &mode_state_startup, &mode_state_idle, &mode_state_attract, &mode_state_prepare, &mode_state_ready, &mode_state_starting, &mode_state_running, &mode_state_over, &mode_state_abort};
static fsm_t mode_fsm;

static const mode_t *current_mode;
static bool run_common_service = false;
mode_storage_t mode_storage;

void mode_init(void) {
  uint8_t selected;
  memset(&mode_storage, 0, sizeof(mode_storage_t));

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

  current_mode = &mode_definitions[5];

  for (uint8_t i = 0; i < MODE_STATE_COUNT; i++) {
    states[i]->enter = (*current_mode->state_funcs)[i].enter;
    states[i]->service = (*current_mode->state_funcs)[i].service;
    states[i]->exit = (*current_mode->state_funcs)[i].exit;
  }

  /* Initialise FSM. */
  fsm_init(&mode_fsm, &mode_state_init, NULL);
}

void mode_service(void) {
  if (run_common_service && current_mode->common_service != NULL) {
    current_mode->common_service();
  }

  fsm_service(&mode_fsm);
}

void mode_enable_common_service(void) {
  run_common_service = true;
}
