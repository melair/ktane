#include <xc.h>
#include <stdbool.h>
#include "simon.h"
#include "../../mode.h"

/* Local function prototypes. */
void simon_service(bool first);

/**
 * Initialise the simon says puzzle.
 */
void simon_initialise(void) {
    /* Register our callbacks. */
    mode_register_callback(GAME_ALWAYS, simon_service, NULL);
}

/**
 * Service the simon says puzzle. 
 * 
 * @param first true if the service routine is called for the first time
 */
void simon_service(bool first) {
    
}