#ifndef FAT_H
#define	FAT_H

#include <stdbool.h>
#include "../../opts.h"

typedef struct fat_lookup_t fat_lookup_t;

struct fat_lookup_t {
    uint8_t mode;
    uint8_t index;

    bool found;

    uint32_t offset;
    uint16_t size;
    uint16_t last_block_usage;

    fat_lookup_t *next;
};

void fat_initialise(opt_data_t *sdcard);
void fat_add(fat_lookup_t *lu);
void fat_scan(void);

#endif	/* FAT_H */

