#include <xc.h>
#include "controller.h"
#include "../../peripherals/timer/segment.h"

/**
 * Initialise any components or state that the controller will require.
 */
void controller_initialise(void) {
    /* Initialise the seven segment display. */
    segment_initialise();
}

/**
 * Service the controllers behaviour.
 */
void controller_service(void) {
    /* Service the seven segment display. */
    segment_service();
}