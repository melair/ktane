#ifndef STATUS_H
#define STATUS_H

#include "../hal/pin.h"
#include <stdint.h>


void status_init(pin_t led, pin_t button, uint8_t max_option, void (*callback)(uint8_t));
void status_service(void);

#endif