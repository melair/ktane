#include <xc.h>
#include "malloc.h"

#define MALLOC_TOP      0x31FF
#define MALLOC_SIZE     0x1200

uint16_t malloc_last = MALLOC_TOP;

void *kmalloc(uint16_t size) {
    malloc_last -= size;

    if (malloc_last <= (MALLOC_TOP - MALLOC_SIZE)) {
        return NULL;
    }

    return (void *) malloc_last;
}