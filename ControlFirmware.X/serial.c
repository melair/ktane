#include <xc.h>
#include <stdint.h>
#include "serial.h"
#include "nvm.h"
#include "rng.h"

/* Module serial number, derived from DIA/MUI. */
uint32_t serial_number;
        
void serial_initialise(void) {
    /* Cycle through MUI words. */
    for (uint8_t i = 0; i < 16; i += 2) {
        uint32_t mui = nvm_read_pfm(DIA_MUI + i);
        
        if (i % 4 == 0) {
            mui <<= 16;
        }
        
        serial_number ^= mui;   
    } 
}

uint32_t serial_get(void) {
    return serial_number;
}
