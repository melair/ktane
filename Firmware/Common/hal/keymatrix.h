#ifndef KEYMATRIX_H
#define KEYMATRIX_H

#include "pin.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    bool last_read;

    bool current_state;
    uint8_t current_state_read_count;

    uint32_t last_state_change;
} keymatrix_state_t;

typedef struct {
    uint8_t i;
    uint8_t event;
    uint16_t duration;
} keymatrix_event_t;

#define KEYMATRIX_EVENT_HISTORY 4

typedef struct {
    pin_t *sense;
    pin_t *drive;
    uint8_t drive_count;

    keymatrix_state_t *state;

    keymatrix_event_t events[KEYMATRIX_EVENT_HISTORY];
    uint8_t event_write;
    uint8_t event_read;

    uint8_t options;
} keymatrix_t;

/* A pressed key will have a high at the input pin. */
#define KEYMATRIX_PRESSED_HIGH 0b00000000
/* A pressed key will be low at the input pin, implicit if using drive pins.*/
#define KEYMATRIX_PRESSED_LOW 0b00000001
/* Don't enable pull up on sense pins, use if not using drive and hardware pull ups are present. */
#define KEYMATRIX_SENSE_NO_PULL_UPS 0b00000010

/* Event subscriptions. */
#define KEYMATRIX_EVENTS_DOWN 0b00000100
#define KEYMATRIX_EVENTS_UP   0b00001000

/* Debound counts. */
#define KEYMATRIX_DEBOUNCE_MASK 0b11100000
#define KEYMATRIX_DEBOUNCE_SHIFT 5
#define KEYMATRIX_DEBOUNCE_10MS 0b00000000
#define KEYMATRIX_DEBOUNCE_20MS 0b00100000
#define KEYMATRIX_DEBOUNCE_30MS 0b01000000
#define KEYMATRIX_DEBOUNCE_40MS 0b01100000
#define KEYMATRIX_DEBOUNCE_50MS 0b10000000
#define KEYMATRIX_DEBOUNCE_60MS 0b10100000
#define KEYMATRIX_DEBOUNCE_70MS 0b11000000
#define KEYMATRIX_DEBOUNCE_80MS 0b11100000

void keymatrix_init(keymatrix_t *km, keymatrix_state_t *kms, pin_t *sense, pin_t *drive, uint8_t options);
void keymatrix_service(keymatrix_t *km);
void keymatrix_clear_events(keymatrix_t *km);
keymatrix_event_t *keymatric_get_event(keymatrix_t *km);

#endif