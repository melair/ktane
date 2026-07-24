#include "rng.h"

#include "stm32h5xx_hal.h"
#include "stm32h5xx_it.h"

static RNG_HandleTypeDef hrng = {0};

/**
  * @brief RNG MSP Initialization
  * This function configures the hardware resources used in this example
  * @param hrng: RNG handle pointer
  * @retval None
  */
void HAL_RNG_MspInit(RNG_HandleTypeDef *hrng) {
    if (hrng->Instance == RNG) {
        /* Initializes the peripherals clock */
        __HAL_RCC_RNG_CONFIG(RCC_RNGCLKSOURCE_HSI48);

        /* Peripheral clock enable */
        __HAL_RCC_RNG_CLK_ENABLE();
    }
}

/**
  * @brief RNG MSP De-Initialization
  * This function freeze the hardware resources used in this example
  * @param hrng: RNG handle pointer
  * @retval None
  */
void HAL_RNG_MspDeInit(RNG_HandleTypeDef *hrng) {
    if (hrng->Instance == RNG) {
        /* Peripheral clock disable */
        __HAL_RCC_RNG_CLK_DISABLE();
    }
}

void TRNG_Init(void) {
    hrng.Instance = RNG;
    hrng.Init.ClockErrorDetection = RNG_CED_ENABLE;

    if (HAL_RNG_Init(&hrng) != HAL_OK) {
        Error_Handler();
    }
}
