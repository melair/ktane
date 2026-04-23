#ifndef RNG_H
#define RNG_H

#include <stdint.h>

uint32_t rng_generate(uint32_t *seed, uint32_t mask);

#endif