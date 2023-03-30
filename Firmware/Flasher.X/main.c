#include <xc.h>
#include <stdint.h>
#include "../common/device.h"

asm ("PSECT flasherversion");
asm ("dw 0x0001");

void main(void) {
    while(1);
}

