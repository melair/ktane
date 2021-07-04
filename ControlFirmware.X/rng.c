#include <xc.h>
#include <stdint.h>
#include "rng.h"

/**
 * Generate a random number based upon the seed provided.
 * 
 * WARNING: This is Xorshift32 - it is not cryptographically secure - don't use
 * it for anything important. https://en.wikipedia.org/wiki/Xorshift
 * 
 * @param seed pointer to seed, will be mutated by function
 * @param mask a mask value to XOR the seed on return, obsfucates the seed to 
 *        consumers of data - this allows multiple systems to generate different
 *        data from the same seed
 * @return 32 bits random of data
 */
uint32_t rng_generate(uint32_t *seed, uint32_t mask) {
    /* Copy seed locally. */
    uint32_t local_seed = *seed;
    
    /* Xorshift32 RNG implemenation. */
    local_seed ^= local_seed << 13;
	local_seed ^= local_seed >> 17;
	local_seed ^= local_seed << 5;  
    
    /* Mutate original seed. */
    *seed = local_seed;
    
    /* Return the new seed xor'd with the application mask. */
    return local_seed ^ mask;
}
