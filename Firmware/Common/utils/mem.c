#include <stddef.h>
#include <xc.h>
#include <stdint.h>

void memset(void *ptr, uint8_t val, uint16_t len) {
    uint8_t *cptr = (uint8_t *)ptr;

    for (size_t i = 0; i < len; i++) {
        cptr[i] = val;
    }
}