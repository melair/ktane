#ifndef GPIO_H
#define GPIO_H

#include "stm32h5xx_hal.h"

typedef struct {
    GPIO_TypeDef *port;
    uint16_t pin;
} GPIO_PinDef;

#define GPIO_SUBMODULE_PIN_COUNT 8

#define BUTTON_Pin GPIO_PIN_7
#define BUTTON_Port GPIOB

#define STATUS_Pin GPIO_PIN_9
#define STATUS_Port GPIOB

#define I2C_SCL_Pin GPIO_PIN_10
#define I2C_SCL_Port GPIOB
#define I2C_SDA_Pin GPIO_PIN_12
#define I2C_SDA_Port GPIOB

#define SPI_SCK_Pin GPIO_PIN_12
#define SPI_SCK_Port GPIOE
#define SPI_MISO_Pin GPIO_PIN_13
#define SPI_MISO_Port GPIOE
#define SPI_MOSI_Pin GPIO_PIN_14
#define SPI_MOSI_Port GPIOE

#define FDCAN_RX_Pin GPIO_PIN_0
#define FDCAN_RX_Port GPIOD
#define FDCAN_TX_Pin GPIO_PIN_1
#define FDCAN_TX_Port GPIOD
#define FDCAN_ACT_Pin GPIO_PIN_3
#define FDCAN_ACT_Port GPIOD

#define ARGB_Pin GPIO_PIN_12
#define ARGB_Port GPIOD

#define SDMMC_D0_Pin GPIO_PIN_8
#define SDMMC_D0_Port GPIOC
#define SDMMC_D1_Pin GPIO_PIN_9
#define SDMMC_D1_Port GPIOC
#define SDMMC_DET_Pin GPIO_PIN_8
#define SDMMC_DET_Port GPIOA
#define SDMMC_PWR_Pin GPIO_PIN_10
#define SDMMC_PWR_Port GPIOD
#define SDMMC_D2_Pin GPIO_PIN_10
#define SDMMC_D2_Port GPIOC
#define SDMMC_D3_Pin GPIO_PIN_11
#define SDMMC_D3_Port GPIOC
#define SDMMC_CK_Pin GPIO_PIN_12
#define SDMMC_CK_Port GPIOC
#define SDMMC_CMD_Pin GPIO_PIN_2
#define SDMMC_CMD_Port GPIOD
#define SDMMC_ACT_Pin GPIO_PIN_10
#define SDMMC_ACT_Port GPIOA

#define USB_DM_Pin GPIO_PIN_11
#define USB_DM_Port GPIOA
#define USB_DP_Pin GPIO_PIN_12
#define USB_DP_Port GPIOA

#define CBUS_RX_Pin GPIO_PIN_5
#define CBUS_RX_Port GPIOB
#define CBUS_TX_Pin GPIO_PIN_6
#define CBUS_TX_Port GPIOB

#define GPIO_A7_Pin GPIO_PIN_8
#define GPIO_A7_Port GPIOE
#define GPIO_A6_Pin GPIO_PIN_9
#define GPIO_A6_Port GPIOE
#define GPIO_A5_Pin GPIO_PIN_10
#define GPIO_A5_Port GPIOE
#define GPIO_A4_Pin GPIO_PIN_11
#define GPIO_A4_Port GPIOE
#define GPIO_A3_Pin GPIO_PIN_9
#define GPIO_A3_Port GPIOD
#define GPIO_A2_Pin GPIO_PIN_8
#define GPIO_A2_Port GPIOD
#define GPIO_A1_Pin GPIO_PIN_14
#define GPIO_A1_Port GPIOB
#define GPIO_A0_Pin GPIO_PIN_15
#define GPIO_A0_Port GPIOB

#define GPIO_B7_Pin GPIO_PIN_1
#define GPIO_B7_Port GPIOB
#define GPIO_B6_Pin GPIO_PIN_0
#define GPIO_B6_Port GPIOB
#define GPIO_B5_Pin GPIO_PIN_5
#define GPIO_B5_Port GPIOC
#define GPIO_B4_Pin GPIO_PIN_4
#define GPIO_B4_Port GPIOC
#define GPIO_B3_Pin GPIO_PIN_7
#define GPIO_B3_Port GPIOA
#define GPIO_B2_Pin GPIO_PIN_6
#define GPIO_B2_Port GPIOA
#define GPIO_B1_Pin GPIO_PIN_5
#define GPIO_B1_Port GPIOA
#define GPIO_B0_Pin GPIO_PIN_4
#define GPIO_B0_Port GPIOA

#define GPIO_C7_Pin GPIO_PIN_3
#define GPIO_C7_Port GPIOC
#define GPIO_C6_Pin GPIO_PIN_2
#define GPIO_C6_Port GPIOC
#define GPIO_C5_Pin GPIO_PIN_1
#define GPIO_C5_Port GPIOC
#define GPIO_C4_Pin GPIO_PIN_0
#define GPIO_C4_Port GPIOC
#define GPIO_C3_Pin GPIO_PIN_3
#define GPIO_C3_Port GPIOA
#define GPIO_C2_Pin GPIO_PIN_2
#define GPIO_C2_Port GPIOA
#define GPIO_C1_Pin GPIO_PIN_1
#define GPIO_C1_Port GPIOA
#define GPIO_C0_Pin GPIO_PIN_0
#define GPIO_C0_Port GPIOA

extern const GPIO_PinDef GPIO_A_Pins[GPIO_SUBMODULE_PIN_COUNT];
extern const GPIO_PinDef GPIO_B_Pins[GPIO_SUBMODULE_PIN_COUNT];
extern const GPIO_PinDef GPIO_C_Pins[GPIO_SUBMODULE_PIN_COUNT];
extern const GPIO_PinDef GPIO_Button_Pin;

#endif //GPIO_H
