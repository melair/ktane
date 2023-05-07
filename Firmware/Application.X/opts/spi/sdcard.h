#ifndef SDCARD_H
#define	SDCARD_H

#include "../../opts.h"

void sdcard_initialise(opt_data_t *opt);
uint8_t sdcard_transaction_callback(sd_transaction_t *t);
void sdcard_transaction_read_block(sd_transaction_t *t, opt_data_t *opt, uint8_t *buffer, uint32_t block);
bool sdcard_is_ready(opt_data_t *opt);

#define SDCARD_SECTOR_SIZE 512

#endif	/* SDCARD_H */

