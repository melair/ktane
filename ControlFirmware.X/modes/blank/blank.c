#include <xc.h>
#include <stdbool.h>
#include "blank.h"
#include "../../argb.h"
#include "../../tick.h"

void blank_initialise(void) {
    /* Toggle between red and blue for blank. */
    while(true) {
        argb_set(0, 31, 0xff, 0x00, 0x00);
        argb_service();
        tick_wait(250);
        argb_set(0, 31, 0x00, 0x00, 0xff);
        argb_service();
        tick_wait(250);
    }
}
