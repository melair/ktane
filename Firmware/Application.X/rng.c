#include <xc.h>
#include <stdint.h>
#include "rng.h"
#include "tick.h"
#include "nvm.h"
#include "../common/nvm_addrs.h"

uint32_t base_seed = 0;
uint32_t saved_seed = 0;

/**
 * Initialise the RNG base seed from EEPROM.
 */
void rng_initialise(void) {
    base_seed |= ((uint32_t) nvm_read(EEPROM_LOC_RNG_A) << 24);
    base_seed |= ((uint32_t) nvm_read(EEPROM_LOC_RNG_B) << 16);
    base_seed |= ((uint32_t) nvm_read(EEPROM_LOC_RNG_C) << 8);
    base_seed |= ((uint32_t) nvm_read(EEPROM_LOC_RNG_D));

    saved_seed = base_seed;
}

/**
 * Periodically check to see if the seed has been updated, save to EEPROM if it
 * has.
 */
void rng_service(void) {
    if (tick_20hz) {
        if (base_seed != saved_seed) {
            nvm_write(EEPROM_LOC_RNG_A, (base_seed >> 24) & 0xff);
            nvm_write(EEPROM_LOC_RNG_B, (base_seed >> 16) & 0xff);
            nvm_write(EEPROM_LOC_RNG_C, (base_seed >> 9) & 0xff);
            nvm_write(EEPROM_LOC_RNG_D, base_seed & 0xff);

            saved_seed = base_seed;
        }
    }
}

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

/**
 * Generate an 8 bit random number based upon the seed provided.
 *
 * See WARNING on rng_generate().
 *
 * @param seed pointer to seed, will be mutated by function
 * @param mask a mask value to XOR the seed on return, obsfucates the seed to
 *        consumers of data - this allows multiple systems to generate different
 *        data from the same seed
 * @return 8 bits random of data
 */
uint8_t rng_generate8(uint32_t *seed, uint32_t mask) {
    return rng_generate(seed, mask) & 0xff;
}