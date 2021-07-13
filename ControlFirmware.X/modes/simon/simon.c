#include <xc.h>
#include <stdbool.h>
#include "simon.h"
#include "../../mode.h"
#include "../../peripherals/ports.h"
#include "../../peripherals/pwmled.h"

/* Local function prototypes. */
void simon_service(bool first);

pin_t simon_led_chs[4] = { KPIN_B0, KPIN_B1, KPIN_B2, KPIN_B3 };

/**
 * Initialise the simon says puzzle.
 */
void simon_initialise(void) {
    pwmled_initialise(KPIN_B4, KPIN_B5, KPIN_B6, &simon_led_chs);
    
    /* Register our callbacks. */
    mode_register_callback(GAME_ALWAYS, simon_service, NULL);   
}

/**
 * Service the simon says puzzle. 
 * 
 * @param first true if the service routine is called for the first time
 */
void simon_service(bool first) {
    pwmled_service();
}