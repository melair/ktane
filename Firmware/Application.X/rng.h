#ifndef RNG_H
#define	RNG_H

void rng_initialise(void);
void rng_service(void);
uint8_t rng_generate8(uint32_t *seed, uint32_t mask);
uint32_t rng_generate(uint32_t *seed, uint32_t mask);

extern uint32_t base_seed;

#endif	/* RNG_H */

