#ifndef RNG_H
#define RNG_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {

#endif

void TRNG_Init(void);

uint32_t TRNG_Rand32(void);

/* Returns a value in the inclusive [minimum, maximum] range. */
uint32_t TRNG_Rand32Range(uint32_t minimum, uint32_t maximum);

uint8_t TRNG_Rand8(void);

/* Returns a value in the inclusive [minimum, maximum] range. */
uint8_t TRNG_Rand8Range(uint8_t minimum, uint8_t maximum);

typedef struct {
    uint32_t state[4];
} PRNG_Seed;

PRNG_Seed PRNG_Init(uint32_t seed);

uint32_t PRNG_Rand32(PRNG_Seed *seed);

/* Returns a value in the inclusive [minimum, maximum] range. */
uint32_t PRNG_Rand32Range(PRNG_Seed *seed, uint32_t minimum, uint32_t maximum);

uint8_t PRNG_Rand8(PRNG_Seed *seed);

/* Returns a value in the inclusive [minimum, maximum] range. */
uint8_t PRNG_Rand8Range(PRNG_Seed *seed, uint8_t minimum, uint8_t maximum);

uint32_t MIXER_LowBias32(uint32_t x);

uint32_t MIXER_SplitMix32(uint32_t *seed);

#ifdef __cplusplus
}
#endif

#endif //RNG_H
