#include "../hal/pin.h"
#include "../utils/time.h"
#include <xc.h>

#define BLINK_INTERVAL 250

uint32_t status_next_change = 0;
bool status_last = false;
pin_t status_led;
pin_t status_button;

void status_init(pin_t led, pin_t button) {
  pin_config(led, OUTPUT, CFG_OPENDRAIN);
  pin_config(button, INPUT, 0);

  status_led = led;
  status_button = button;
}

void status_service(void) {
    if (uptime_in_ms > status_next_change) {
        status_last = !status_last;
        pin_write(status_led, status_last);
        status_next_change = uptime_in_ms + BLINK_INTERVAL;
    }
}