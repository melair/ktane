#include <xc.h>
#include <stdint.h>
#include "../common/mcu.h"
#include "../common/device.h"
#include "../common/fw.h"

/* Flasher Version. */
asm ("PSECT flasherversion");
asm ("dw 0x0001");

void main(void) {
    /* Drive everything unused to low. */
    safe_unused_pins();

    /* Reconfigure system arbiter. */
    arbiter_initialise();

    /* Configure sleep behaviour, put CPU in IDLE. */
    CPUDOZEbits.IDLEN = 1;

    /* Load versions from PFM. */
    fw_initialise();

    /* Unlock PPS during initialisation. */
    pps_unlock();
    
    while(1);
}

