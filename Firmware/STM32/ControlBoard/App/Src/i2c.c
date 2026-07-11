#include "i2c.h"
#include "gpio.h"
#include "stm32h5xx_hal.h"
#include "stm32h5xx_it.h"

/**
  * @brief I2C MSP Initialization
  * This function configures the hardware resources used in this example
  * @param hi2c: I2C handle pointer
  * @retval None
  */
void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (hi2c->Instance == I2C2) {
        /* Initializes the peripherals clock */
        __HAL_RCC_I2C2_CONFIG(RCC_I2C2CLKSOURCE_PCLK1);

        __HAL_RCC_GPIOB_CLK_ENABLE();
        /* I2C2 GPIO Configuration
         * PB10     ------> I2C2_SCL
         * PB12     ------> I2C2_SDA
         */
        GPIO_InitStruct.Pin = I2C_SCL_Pin | I2C_SDA_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF4_I2C2;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        /* Peripheral clock enable */
        __HAL_RCC_I2C2_CLK_ENABLE();
    }
}

/**
  * @brief I2C MSP De-Initialization
  * This function freeze the hardware resources used in this example
  * @param hi2c: I2C handle pointer
  * @retval None
  */
void HAL_I2C_MspDeInit(I2C_HandleTypeDef *hi2c) {
    if (hi2c->Instance == I2C2) {
        /* Peripheral clock disable */
        __HAL_RCC_I2C2_CLK_DISABLE();

        /* I2C2 GPIO Configuration
         * PB10     ------> I2C2_SCL
         * PB12     ------> I2C2_SDA
         */
        HAL_GPIO_DeInit(GPIOB, I2C_SCL_Pin | I2C_SDA_Pin);
    }
}

void I2C_Init(void) {
    I2C_HandleTypeDef hi2c2 = {0};

    /* Initialise I2C2 peripheral at 100kHz. */
    hi2c2.Instance = I2C2;
    hi2c2.Init.Timing = 0x60808CD3;
    hi2c2.Init.OwnAddress1 = 0;
    hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c2.Init.OwnAddress2 = 0;
    hi2c2.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c2) != HAL_OK) {
        Error_Handler();
    }

    if (HAL_I2CEx_ConfigAnalogFilter(&hi2c2, I2C_ANALOGFILTER_ENABLE) != HAL_OK) {
        Error_Handler();
    }

    if (HAL_I2CEx_ConfigDigitalFilter(&hi2c2, 0) != HAL_OK) {
        Error_Handler();
    }
}
