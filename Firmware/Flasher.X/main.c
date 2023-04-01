#include <xc.h>
#include <stdint.h>
#include "../common/device.h"
#include "../common/fw.h"

/* Flasher Version. */
asm ("PSECT flasherversion");
asm ("dw 0x0001");

void main(void) {
    /* Initialise versions. */
    fw_initialise();
    
    while(1);
}

