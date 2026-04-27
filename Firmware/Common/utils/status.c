#include "status.h"
#include "../hal/keymatrix.h"
#include "../hal/pin.h"
#include "fsm.h"
#include "time.h"
#include <stdbool.h>
#include <stdint.h>
#include <xc.h>

#define IDLE_BLINK_INTERVAL 250

#define MENU_ENTER_TIME 2000
#define MENU_EXIT_TIME 15000

#define MENU_GAP_TIME 1000
#define MENU_BLINK_INTERVAL 350

typedef struct {
  fsm_t fsm;
  keymatrix_t keymatrix;
  keymatrix_state_t keymatrix_state[1];
  pin_t keymatrix_sense[2];

  uint8_t max_option;

  void (*callback)(uint8_t);

  union {
    struct {
      uint32_t next;
      bool state;
    } blink;
    struct {
      uint32_t enter_at;
      uint8_t option;

      uint32_t display_next_at;
      uint8_t display_option;
      uint8_t display_phase;
    } menu;
  };

  pin_t led;
} status_t;

extern const fs_t status_state_idle_blink;
extern const fs_t status_state_menu;

static status_t status = {.fsm = {.initial = &status_state_idle_blink}};

void status_state_idle_enter(fsm_t *fsm) {
  status.blink.state = false;
  status.blink.next = 0;
}

void status_state_idle_service(fsm_t *fsm) {
  if (uptime_in_ms > status.blink.next) {
    status.blink.state = !status.blink.state;
    pin_write(status.led, status.blink.state);
    status.blink.next = uptime_in_ms + IDLE_BLINK_INTERVAL;
  }

  for (keymatrix_event_t *event = keymatric_get_event(&status.keymatrix);
       event != NULL; event = keymatric_get_event(&status.keymatrix)) {
    if (event->event == KEYMATRIC_EVENTS_UP &&
        event->duration >= MENU_ENTER_TIME) {
      fsm_transition(fsm, &status_state_menu);
    }
  }
}

const fs_t status_state_idle_blink = {
    .enter = status_state_idle_enter,
    .service = status_state_idle_service,
    .next_states = {&status_state_menu, NULL}};

void status_state_menu_enter(fsm_t *fsm) {
  status.menu.enter_at = uptime_in_ms;
  status.menu.option = 1;
  status.menu.display_option = 1;
  status.menu.display_next_at = 0;
  status.menu.display_phase = 0;
}

void status_state_menu_service(fsm_t *fsm) {
  if (status.menu.display_next_at <= uptime_in_ms) {
    if (status.menu.display_phase == 0) {
      pin_write(status.led, false);
      status.menu.display_next_at = uptime_in_ms + MENU_BLINK_INTERVAL;
      status.menu.display_phase++;
    } else {
      pin_write(status.led, true);

      if (status.menu.display_option >= status.menu.option) {
        status.menu.display_next_at = uptime_in_ms + MENU_GAP_TIME;
        status.menu.display_option = 1;
      } else {
        status.menu.display_next_at = uptime_in_ms + MENU_BLINK_INTERVAL;
        status.menu.display_option++;
      }
      status.menu.display_phase = 0;
    }
  }

  for (keymatrix_event_t *event = keymatric_get_event(&status.keymatrix);
       event != NULL; event = keymatric_get_event(&status.keymatrix)) {
    if (event->event == KEYMATRIC_EVENTS_UP) {
      status.menu.enter_at = uptime_in_ms;

      if (event->duration <= MENU_ENTER_TIME) {
        status.menu.option++;
        if (status.menu.option > status.max_option) {
          status.menu.option = 1;
        }
      } else {
        if (status.callback != NULL) {
          status.callback(status.menu.option);
        }

        fsm_transition(fsm, &status_state_idle_blink);
      }
    }
  }

  if (uptime_in_ms > (status.menu.enter_at + MENU_EXIT_TIME)) {
    fsm_transition(fsm, &status_state_idle_blink);
  }
}

const fs_t status_state_menu = {
    .enter = status_state_menu_enter,
    .service = status_state_menu_service,
    .next_states = {&status_state_idle_blink, NULL}};

void status_init(pin_t led, pin_t button, uint8_t max_option, void (*callback)(uint8_t)) {
  pin_config(led, OUTPUT, CFG_OPENDRAIN);
  pin_config(button, INPUT, 0);

  status.keymatrix_sense[0] = button;
  status.keymatrix_sense[1] = PORTPIN_NONE;

  keymatrix_init(&status.keymatrix, &status.keymatrix_state[0],
                 &status.keymatrix_sense[0], NULL,
                 KEYMATRIX_PRESSED_LOW | KEYMATRIX_SENSE_NO_PULL_UPS |
                     KEYMATRIX_DEBOUNCE_50MS | KEYMATRIC_EVENTS_UP);

  status.led = led;
  status.max_option = max_option;
  status.callback = callback;

  fsm_init(&status.fsm);
}

void status_service(void) {
  keymatrix_service(&status.keymatrix);
  fsm_service(&status.fsm);
}