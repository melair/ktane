#ifndef GPIO_H
#define GPIO_H

#define BUTTON_Pin GPIO_PIN_7
#define BUTTON_GPIO_Port GPIOB

#define STATUS_Pin GPIO_PIN_9
#define STATUS_GPIO_Port GPIOB

#define I2C_SCL_Pin GPIO_PIN_10
#define I2C_SCL_GPIO_Port GPIOB
#define I2C_SDA_Pin GPIO_PIN_12
#define I2C_SDA_GPIO_Port GPIOB

#define SPI_SCK_Pin GPIO_PIN_12
#define SPI_SCK_GPIO_Port GPIOE
#define SPI_MISO_Pin GPIO_PIN_13
#define SPI_MISO_GPIO_Port GPIOE
#define SPI_MOSI_Pin GPIO_PIN_14
#define SPI_MOSI_GPIO_Port GPIOE

#define FDCAN_RX_Pin GPIO_PIN_0
#define FDCAN_RX_GPIO_Port GPIOD
#define FDCAN_TX_Pin GPIO_PIN_1
#define FDCAN_TX_GPIO_Port GPIOD

#define FDCAN_ACT_Pin GPIO_PIN_3
#define FDCAN_ACT_GPIO_Port GPIOD

#define ARGB_Pin GPIO_PIN_12
#define ARGB_GPIO_Port GPIOD

#endif //GPIO_H
