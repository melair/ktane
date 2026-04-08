#ifndef GPIO_H
#define	GPIO_H

#include <hal/pin.h>

/* Front Interface */
#define GPIO_FRONT_MODULE_DETECT   (PORT_C | PIN_6)
#define GPIO_FRONT_EFUSE_EN        (PORT_B | PIN_3)
#define GPIO_FRONT_EFUSE_FLT       (PORT_B | PIN_2)
#define GPIO_FRONT_BOOT            (PORT_B | PIN_1)
#define GPIO_FRONT_UART_TX         (PORT_C | PIN_7)
#define GPIO_FRONT_UART_RX         (PORT_B | PIN_0)

/* Rear Interface */
#define GPIO_REAR_MODULE_DETECT    (PORT_A | PIN_2)
#define GPIO_REAR_EFUSE_EN         (PORT_A | PIN_1)
#define GPIO_REAR_EFUSE_FLT        (PORT_A | PIN_0)
#define GPIO_REAR_BOOT             (PORT_A | PIN_3)
#define GPIO_REAR_UART_TX          (PORT_A | PIN_5)
#define GPIO_REAR_UART_RX          (PORT_A | PIN_4)

/* Control Board integrated peripherals and pins. */
#define GPIO_BOOT                  (PORT_C | PIN_1)
#define GPIO_STATUS                (PORT_C | PIN_0)

#define GPIO_SDA                   (PORT_B | PIN_5)
#define GPIO_SCL                   (PORT_B | PIN_4)

#define GPIO_BUS_UART_TX           (PORT_C | PIN_3)
#define GPIO_BUS_UART_RX           (PORT_C | PIN_4)
#define GPIO_BUS_UART_DE           (PORT_C | PIN_2)

#endif