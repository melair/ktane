#ifndef GPIO_H
#define	GPIO_H

#include <hal/pin/pin.h>

#define GPIO_0          (PORT_B | PIN_6)
#define GPIO_1          (PORT_B | PIN_5)
#define GPIO_SCL        (PORT_B | PIN_4)
#define GPIO_SDA        (PORT_C | PIN_2)
#define GPIO_4          (PORT_A | PIN_2)
#define GPIO_5          (PORT_C | PIN_0)
#define GPIO_6          (PORT_C | PIN_1)
#define GPIO_7          (PORT_A | PIN_5)
#define GPIO_8          (PORT_A | PIN_4)
#define GPIO_9          (PORT_C | PIN_5)
#define GPIO_10         (PORT_C | PIN_4)
#define GPIO_11         (PORT_C | PIN_3)

#define GPIO_UART_RX    (PORT_B | PIN_7)
#define GPIO_UART_TX    (PORT_C | PIN_6)
#define GPIO_UART_DE    (PORT_C | PIN_7)

#endif