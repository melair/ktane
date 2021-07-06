#ifndef RNG_H
#define	RNG_H

uint8_t rng_generate8(uint32_t *seed, uint32_t mask);
uint32_t rng_generate(uint32_t *seed, uint32_t mask);

#endif	/* RNG_H */

