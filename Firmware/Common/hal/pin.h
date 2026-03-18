#ifndef PIN_H
#define	PIN_H

#include <stdint.h>
#include <stdbool.h>

typedef uint8_t pin_t;

#define INPUT 1
#define OUTPUT 0

#define CFG_PULLUP      0b00000001
#define CFG_OPENDRAIN   0b00000010
#define CFG_ANALOG      0b00000100

void pin_config(pin_t pin, uint8_t direction, uint8_t config);
void pin_write(pin_t pin, bool value);
bool pin_read(pin_t pin);
volatile uint8_t *pin_to_pps(pin_t pin);

#define PORT_A 0b00000000
#define PORT_B 0b00001000
#define PORT_C 0b00010000
#define PORT_D 0b00011000
#define PORT_E 0b00100000
#define PORT_F 0b00101000

#define PIN_0  0b00000000
#define PIN_1  0b00000001
#define PIN_2  0b00000010
#define PIN_3  0b00000011
#define PIN_4  0b00000100
#define PIN_5  0b00000101
#define PIN_6  0b00000110
#define PIN_7  0b00000111

#endif