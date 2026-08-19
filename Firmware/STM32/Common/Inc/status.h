#ifndef STATUS_H
#define STATUS_H

#include <stdbool.h>
#include <stdint.h>

#include "sys/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*Status_SelectionCallback)(uint8_t value);

/**
 * Initializes the status heartbeat and button-driven menu.
 * button_active_high selects whether a high GPIO level represents a pressed button.
 * Menu values are one-based, from 1 through max_menu_value.
 */
bool Status_Init(GPIO_PinDef led_pin, GPIO_PinDef button_pin, bool button_active_high, uint8_t max_menu_value,
                 Status_SelectionCallback selection_callback);

void Status_Service(void);

#ifdef __cplusplus
}
#endif

#endif //STATUS_H
