#include <xc.h>
#include <stdint.h>
#include "../common/device.h"
#include "../common/versions.h"

/* Flasher Version. */
asm ("PSECT flasherversion");
asm ("dw 0x0001");

void main(void) {
    /* Initialise versions. */
    versions_initialise();
    
    while(1);
}

