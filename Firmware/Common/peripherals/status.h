#ifndef STATUS_H
#define STATUS_H

void status_init(pin_t led, pin_t button, uint8_t options);
void status_service(void);

#endif