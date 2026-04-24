#include "../hal/pin.h"
#include "../utils/fsm.h"
#include "../utils/time.h"
#include <stdbool.h>
#include <stdint.h>
#include <xc.h>

#define IDLE_BLINK_INTERVAL 250

#define MENU_ENTER_TIME 2000
#define MENU_EXIT_TIME 10000

typedef struct {
  fsm_t fsm;

  uint8_t options;

  uint32_t blink_next;
  bool blink_state;

  bool button_last_state;
  uint32_t button_last_change;

  pin_t led;
  pin_t button;
} status_t;

extern const fs_t status_state_idle_blink;
extern const fs_t status_state_menu;

static status_t status = {.blink_state = false,
                          .blink_next = 0,
                          .fsm = {.initial = &status_state_idle_blink}};

void status_state_idle_service(fsm_t *fsm) {
  if (uptime_in_ms > status.blink_next) {
    status.blink_state = !status.blink_state;
    pin_write(status.led, status.blink_state);
    status.blink_next = uptime_in_ms + IDLE_BLINK_INTERVAL;
  }

  bool button_state = !pin_read(status.button);
  if (button_state != status.button_last_state) {
    if (status.button_last_state) {
      uint32_t t = uptime_in_ms - status.button_last_change;

      if (t >= MENU_ENTER_TIME) {
        fsm_transition(fsm, &status_state_menu);
      }
    }

    status.button_last_change = uptime_in_ms;
    status.button_last_state = button_state;
  }
}

const fs_t status_state_idle_blink = {
    .service = status_state_idle_service,
    .next_states = {&status_state_menu, NULL}};

void status_state_menu_service(fsm_t *fsm) {

  bool button_state = !pin_read(status.button);
  if (!button_state) {
    uint32_t t = uptime_in_ms - status.button_last_change;

    if (t >= MENU_EXIT_TIME) {
      fsm_transition(fsm, &status_state_idle_blink);
    }
  }
}

const fs_t status_state_menu = {
    .service = status_state_menu_service,
    .next_states = {&status_state_idle_blink, NULL}};

void status_init(pin_t led, pin_t button, uint8_t options) {
  pin_config(led, OUTPUT, CFG_OPENDRAIN);
  pin_config(button, INPUT, 0);

  status.led = led;
  status.button = button;
  status.options = options;

  fsm_init(&status.fsm);

  if (uptime_in_ms == 0xffffffff) {
    status_state_idle_service(NULL);
    status_state_menu_service(NULL);
  }
}

void status_service(void) { fsm_service(&status.fsm); }