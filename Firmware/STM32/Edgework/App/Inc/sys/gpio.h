#ifndef GPIO_H
#define GPIO_H

#include "stm32g0xx_hal.h"

typedef struct {
    GPIO_TypeDef *port;
    uint16_t pin;
} GPIO_PinDef;

#define GPIO0_Pin GPIO_PIN_2
#define GPIO0_Port GPIOA
#define GPIO1_Pin GPIO_PIN_1
#define GPIO1_Port GPIOA
#define GPIO4_Pin GPIO_PIN_3
#define GPIO4_Port GPIOA
#define GPIO5_Pin GPIO_PIN_4
#define GPIO5_Port GPIOA
#define GPIO6_Pin GPIO_PIN_5
#define GPIO6_Port GPIOA
#define GPIO7_Pin GPIO_PIN_6
#define GPIO7_Port GPIOA
#define GPIO8_Pin GPIO_PIN_7
#define GPIO8_Port GPIOA
#define GPIO9_Pin GPIO_PIN_0
#define GPIO9_Port GPIOB
#define GPIO10_Pin GPIO_PIN_2
#define GPIO10_Port GPIOB
#define GPIO11_Pin GPIO_PIN_1
#define GPIO11_Port GPIOB

#define ARGB_Pin GPIO4_Pin
#define ARGB_Port GPIO4_Port

#define SPI_SCK_Pin GPIO6_Pin
#define SPI_SCK_Port GPIO6_Port
#define SPI_MISO_Pin GPIO7_Pin
#define SPI_MISO_Port GPIO7_Port
#define SPI_MOSI_Pin GPIO8_Pin
#define SPI_MOSI_Port GPIO8_Port

#define BUS_DE_Pin GPIO_PIN_3
#define BUS_DE_Port GPIOB
#define BUS_TX_Pin GPIO_PIN_6
#define BUS_TX_Port GPIOB
#define BUS_RX_Pin GPIO_PIN_7
#define BUS_RX_Port GPIOB

#define I2C_SCL_Pin GPIO_PIN_8
#define I2C_SCL_Port GPIOB
#define I2C_SDA_Pin GPIO_PIN_9
#define I2C_SDA_Port GPIOB

#endif // GPIO_H
