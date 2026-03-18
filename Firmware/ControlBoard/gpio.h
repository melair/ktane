#ifndef GPIO_H
#define	GPIO_H

#include <hal/pin/pin.h>

/* Submodule A, uses PIC port A, inversed and A0/A1 are flipped. */
#define GPIO_A0         (PORT_A | PIN_6)
#define GPIO_A1         (PORT_A | PIN_7) 
#define GPIO_A2         (PORT_A | PIN_5)
#define GPIO_A3         (PORT_A | PIN_4)
#define GPIO_A4         (PORT_A | PIN_3)
#define GPIO_A5         (PORT_A | PIN_2)
#define GPIO_A6         (PORT_A | PIN_1)
#define GPIO_A7         (PORT_A | PIN_0)

/* Submodule B, uses PIC port F. */
#define GPIO_B0         (PORT_F | PIN_7)
#define GPIO_B1         (PORT_F | PIN_6)
#define GPIO_B2         (PORT_F | PIN_5)
#define GPIO_B3         (PORT_F | PIN_4)
#define GPIO_B4         (PORT_F | PIN_3)
#define GPIO_B5         (PORT_F | PIN_2)
#define GPIO_B6         (PORT_F | PIN_1)
#define GPIO_B7         (PORT_F | PIN_0)

/* Submodule C, uses PIC port C. */
#define GPIO_C0         (PORT_C | PIN_7)
#define GPIO_C1         (PORT_C | PIN_6)
#define GPIO_C2         (PORT_C | PIN_5)
#define GPIO_C3         (PORT_C | PIN_4)
#define GPIO_C4         (PORT_C | PIN_3)
#define GPIO_C5         (PORT_C | PIN_2)
#define GPIO_C6         (PORT_C | PIN_1)
#define GPIO_C7         (PORT_C | PIN_0)

#define GPIO_BOOT       (PORT_E | PIN_0)
#define GPIO_STATUS     (PORT_E | PIN_1)
#define GPIO_CAN_ACT    (PORT_E | PIN_2)

#define GPIO_UART_TX    (PORT_B | PIN_0)
#define GPIO_UART_RX    (PORT_D | PIN_7)

#define GPIO_SDA        (PORT_B | PIN_1)
#define GPIO_SCL        (PORT_B | PIN_2)

#define GPIO_CAN_TX     (PORT_B | PIN_4)
#define GPIO_CAN_RX     (PORT_B | PIN_5)

#define GPIO_ARGB       (PORT_D | PIN_6)

#define GPIO_SPI_CIPO   (PORT_D | PIN_0)
#define GPIO_SPI_CLK    (PORT_D | PIN_2)
#define GPIO_SPI_COPI   (PORT_D | PIN_3)

#define GPIO_SD_CS      (PORT_D | PIN 1)
#define GPIO_SD_PRESENT (PORT_D | PIN_4)
#define GPIO_SD_POWER   (PORT_D | PIN_5)

#endif