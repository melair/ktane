#include "rng.h"

#include "stm32h5xx_hal.h"
#include "stm32h5xx_it.h"
#include <stdint.h>

static RNG_HandleTypeDef hrng = {0};

static uint32_t PRNG_Rotl32(uint32_t x, uint8_t k);

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

uint32_t TRNG_Rand32(void) {
    uint32_t random = 0;

    if (HAL_RNG_GenerateRandomNumber(&hrng, &random) != HAL_OK) {
        Error_Handler();
    }

    return random;
}

uint8_t TRNG_Rand8(void) {
    return (uint8_t) TRNG_Rand32();
}

uint8_t TRNG_Rand8Bound(uint8_t max) {
    if (max == 0U) {
        return 0;
    }

    return (uint8_t) (((uint64_t) TRNG_Rand32() * max) >> 32U);
}

PRNG_Seed PRNG_Init(uint32_t seed) {
    PRNG_Seed prngSeed = {0};

    for (uint8_t i = 0; i < 4U; i++) {
        prngSeed.state[i] = MIXER_SplitMix32(&seed);
    }

    return prngSeed;
}

uint32_t PRNG_Rand32(PRNG_Seed *seed) {
    if (seed == NULL) {
        return 0;
    }

    uint32_t *state = seed->state;
    uint32_t random = PRNG_Rotl32(state[1] * 5U, 7U) * 9U;
    uint32_t t = state[1] << 9U;

    state[2] ^= state[0];
    state[3] ^= state[1];
    state[1] ^= state[2];
    state[0] ^= state[3];

    state[2] ^= t;
    state[3] = PRNG_Rotl32(state[3], 11U);
    return random;
}

uint8_t PRNG_Rand8(PRNG_Seed *seed) {
    return (uint8_t) PRNG_Rand32(seed);
}

uint8_t PRNG_Rand8Bound(PRNG_Seed *seed, uint8_t max) {
    if (max == 0U) {
        return 0;
    }

    return (uint8_t) (((uint64_t) PRNG_Rand32(seed) * max) >> 32U);
}

uint32_t MIXER_LowBias32(uint32_t x) {
    x ^= x >> 16;
    x *= UINT32_C(0x7FEB352D);
    x ^= x >> 15;
    x *= UINT32_C(0x846CA68B);
    x ^= x >> 16;
    return x;
}

uint32_t MIXER_SplitMix32(uint32_t *seed) {
    *seed += UINT32_C(0x9E3779B9);

    uint32_t x = *seed;
    x ^= x >> 16U;
    x *= UINT32_C(0x21F0AAAD);
    x ^= x >> 15U;
    x *= UINT32_C(0x735A2D97);
    x ^= x >> 15U;
    return x;
}

static uint32_t PRNG_Rotl32(uint32_t x, uint8_t k) {
    return (x << k) | (x >> (32U - k));
}