#include <xc.h>
#include "ui.h"
#include "../../rng.h"
#include "../../game.h"

#define GAME_RNG_MASK 0x89b1a96c

uint8_t ui_game_quick_start(uint8_t current, action_t *a) {
    uint32_t seed = rng_generate(&base_seed, GAME_RNG_MASK);

    /* Create the game. */
    game_create(seed, 3, 5, 0);
    
    return a->index;
}