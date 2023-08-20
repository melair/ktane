#include <xc.h>
#include "fat.h"
#include "sdcard.h"
#include "../../hal/spi.h"
#include "../../malloc.h"

typedef struct {
    uint8_t mode;
    uint8_t index;

    struct {
        unsigned unused1 : 4;
        unsigned type : 4;
    };
    uint8_t unused2;

    uint16_t size;
    uint16_t last_block_usage;
} fat_entry_t;

#define FAT_ENTRIES_PER_SECTOR (SDCARD_SECTOR_SIZE / sizeof(fat_entry_t))

fat_lookup_t *fat_lookup_head = NULL;
fat_lookup_t *fat_lookup_tail = NULL;

sd_transaction_t fat_transaction;
void *fat_page = NULL;
bool fat_scan_active = false;
uint8_t fat_scan_page = 0;
uint32_t fat_offset = 0;
opt_data_t *fat_scan_sdcard;

/* Local function prototypes. */
spi_command_t *fat_spi_callback(spi_command_t *cmd);

void fat_initialise(opt_data_t *sdcard) {
    fat_scan_sdcard = sdcard;
    fat_page = kmalloc(SDCARD_SECTOR_SIZE);
}

void fat_add(fat_lookup_t *lu) {
    if (fat_lookup_head == NULL) {
        fat_lookup_head = lu;
    } else {
        fat_lookup_tail->next = lu;
    }

    fat_lookup_tail = lu;
}

void fat_scan(void) {
    if (fat_scan_active) {
        return;
    }

    fat_transaction.spi_cmd.callback = fat_spi_callback;
    fat_transaction.spi_cmd.callback_ptr = NULL;

    sdcard_transaction_read_block(&fat_transaction, fat_scan_sdcard, fat_page, fat_scan_page);
    sdcard_transaction_callback(&fat_transaction);
    spi_enqueue(&fat_transaction.spi_cmd);

    fat_scan_active = true;
    fat_offset = 0;
}

spi_command_t *fat_spi_callback(spi_command_t *cmd) {
    bool next_page;

    switch (sdcard_transaction_callback(&fat_transaction)) {
        case SDCARD_CMD_ERROR:
            return NULL;
        case SDCARD_CMD_COMPLETE:
            next_page = true;

            fat_entry_t *e = (fat_entry_t *) fat_page;
            for (uint8_t i = 0; i < FAT_ENTRIES_PER_SECTOR; i++) {
                if (e->type == 0x0) {
                    next_page = false;
                    break;
                }

                for (fat_lookup_t *p = fat_lookup_head; p != NULL; p = p->next) {
                    if (p->mode == e->mode && p->index == e->index && !p->found) {
                        p->found = true;
                        p->offset = fat_offset;
                        p->size = e->size;
                        p->last_block_usage = e->last_block_usage;
                        break;
                    }
                }

                fat_offset += e->size;
                e++;
            }

            fat_scan_page++;

            if (next_page) {
                sdcard_transaction_read_block(&fat_transaction, fat_scan_sdcard, fat_page, fat_scan_page);
                sdcard_transaction_callback(&fat_transaction);
            } else {
                for (fat_lookup_t *p = fat_lookup_head; p != NULL; p = p->next) {
                    if (p->found) {
                        p->offset += fat_scan_page;
                    }
                }

                fat_scan_page = 0;
                fat_scan_active = false;
            }

            return NULL;
    }

    return cmd;
}
