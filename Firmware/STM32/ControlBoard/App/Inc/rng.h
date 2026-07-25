#ifndef RNG_H
#define RNG_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void TRNG_Init(void);
uint32_t TRNG_Rand32(void);
uint8_t TRNG_Rand8(void);
uint8_t TRNG_Rand8Bound(uint8_t max);

typedef struct {
    uint32_t state[4];
} PRNG_Seed;

PRNG_Seed PRNG_Init(uint32_t seed);
uint32_t PRNG_Rand32(PRNG_Seed *seed);
uint8_t PRNG_Rand8(PRNG_Seed *seed);
uint8_t PRNG_Rand8Bound(PRNG_Seed *seed, uint8_t max);
uint32_t MIXER_LowBias32(uint32_t x);
uint32_t MIXER_SplitMix32(uint32_t *seed);

#ifdef __cplusplus
}
#endif

#endif //RNG_H
