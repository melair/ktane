#include <stdint.h>
#include <xc.h>
#include "uniqueid.h"

uint32_t unique_id;

void unique_init(void) {
    unique_id = 0;

    for (uint8_t i = 0; i < 8; i++) {
        uint32_t value = _DIA_MUI[i];

        if (i % 2 == 0) {
            value <<= 16;
        }

        unique_id ^= value;
    }
}