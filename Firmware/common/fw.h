#ifndef FW_H
#define	FW_H

#include <stdint.h>

void fw_initialise(void);
uint16_t fw_version(uint8_t sgment);
uint32_t fw_checksum(uint8_t segment);
uint16_t fw_page_count(uint8_t segment);
void fw_page_read(uint8_t segment, uint16_t page, uint8_t *data);
void fw_page_write(uint8_t segment, uint16_t page, uint8_t *data);

#endif	/* FW_H */

