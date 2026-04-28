#include "keymatrix.h"
#include "../utils/mem.h"
#include "../utils/time.h"
#include "pin.h"
#include "time.h"
#include <xc.h>

void keymatrix_service_sense(keymatrix_t *km, uint8_t base);
void keymatric_event(keymatrix_t *km, uint8_t i, uint8_t event,
                     uint16_t interval);

void keymatrix_init(keymatrix_t *km, keymatrix_state_t *kms, pin_t *sense,
                    pin_t *drive, uint8_t options) {

  memset(km, 0, sizeof(keymatrix_t));

  km->state = kms;
  km->sense = sense;
  km->drive = drive;

  if (options == 0) {
    options++;
  }

  km->options = options;

  bool sense_pin_opts = 0;
  if ((km->options & KEYMATRIX_SENSE_NO_PULL_UPS) == 0) {
    sense_pin_opts |= CFG_PULLUP;
  }

  if (drive != NULL) {
    km->options |= KEYMATRIX_PRESSED_LOW;

    for (uint8_t d = 0; drive[d] != PORTPIN_NONE; d++) {
      km->drive_count++;
      pin_config(drive[d], OUTPUT, CFG_PULLUP | CFG_OPENDRAIN);
      pin_write(drive[d], true);
    }
  }

  for (uint8_t s = 0; sense[s] != PORTPIN_NONE; s++) {
    pin_config(sense[s], INPUT, sense_pin_opts);
  }
}

void keymatrix_service(keymatrix_t *km) {
  if (!tick_100hz) {
    return;
  }

  if (km->drive != NULL) {
    for (uint8_t d = 0; km->drive[d] != PORTPIN_NONE; d++) {
      pin_write(km->drive[d], false);

      keymatrix_service_sense(km, (d * km->drive_count));

      pin_write(km->drive[d], true);
    }
  } else {
    keymatrix_service_sense(km, 0);
  }
}

void keymatrix_service_sense(keymatrix_t *km, uint8_t base) {
  bool low_pressed = (km->options & KEYMATRIX_PRESSED_LOW) != 0;

  for (uint8_t s = 0; km->sense[s] != PORTPIN_NONE; s++) {
    uint8_t i = base + s;
    keymatrix_state_t *st = &km->state[i];

    bool pressed = pin_read(km->sense[s]) ^ low_pressed;

    if (pressed != st->last_read) {
      st->current_state_read_count = 0;
    } else {
      if (st->current_state_read_count < 0xff) {
        st->current_state_read_count++;
      }

      if (st->current_state_read_count >=
          ((km->options & KEYMATRIX_DEBOUNCE_MASK) >>
           KEYMATRIX_DEBOUNCE_SHIFT) +
              1) {
        if (st->current_state != pressed) {
          uint32_t time = uptime_in_ms - st->last_state_change;
          if (time > 0xffff) {
            time = 0xffff;
          }

          st->current_state = pressed;
          st->current_state_read_count = 0;
          st->last_state_change = uptime_in_ms;

          uint8_t event =
              (pressed ? KEYMATRIX_EVENTS_DOWN : KEYMATRIX_EVENTS_UP);

          if ((km->options & event) != 0) {
            keymatric_event(km, i, event, time & 0xffff);
          }
        }
      }
    }

    st->last_read = pressed;
  }
}

void keymatric_event(keymatrix_t *km, uint8_t i, uint8_t event,
                     uint16_t interval) {
  km->events[km->event_write].i = i;
  km->events[km->event_write].event = event;
  km->events[km->event_write].duration = interval;

  km->event_write++;
  if (km->event_write >= KEYMATRIX_EVENT_HISTORY) {
    km->event_write = 0;
  }

  /* Handle overflowing the key history loop, move the read pointer on. */
  if (km->event_write == km->event_read) {
    km->event_read++;
    if (km->event_read >= KEYMATRIX_EVENT_HISTORY) {
      km->event_read = 0;
    }
  }
}

void keymatrix_clear_events(keymatrix_t *km) {
  km->event_read = 0;
  km->event_write = 0;
}

keymatrix_event_t *keymatric_get_event(keymatrix_t *km) {
  if (km->event_write == km->event_read) {
    return NULL;
  }

  uint8_t ret = km->event_read;

  km->event_read++;
  if (km->event_read >= KEYMATRIX_EVENT_HISTORY) {
    km->event_read = 0;
  }

  return &km->events[ret];
}