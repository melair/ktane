#include "argb.h"
#include "../utils/fsm.h"
#include "hal/pin.h"
#include <xc.h>

typedef struct argb_t {
  fsm_t fsm;
  argb_led_t *buffer;
  uint8_t buffer_len;
};

argb_led_t argb_default_buffer[ARGB_DEFAULT_BUFFER_SIZE];

argb_t argb = {.buffer = &argb_default_buffer[0],
               .buffer_len = ARGB_DEFAULT_BUFFER_SIZE}

void argb_init(pin_t out, bool negate) {
    pin_config(out, OUTPUT, 0);


}

void argb_set_buffer(argb_led_t *buffer, uint8_t len) {
    argb.buffer = buffer;
    argb.buffer_len = len;
}

void argb_set(uint8_t idx, uint8_t r, uint8_t g, uint8_t b) {
    if (idx <= argb.buffer_len) {
        argb.buffer[idx]->R = r;
        argb.buffer[idx]->G = g;
        argb.buffer[idx]->B = b;
    }
}

void argb_update(void) {

}