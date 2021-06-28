#ifndef FIRMWARE_H
#define	FIRMWARE_H

void firmware_initialise(void);
void firmware_flash(void);
void firmware_check(uint16_t);
void firmware_header_received(uint8_t id, uint16_t version, uint16_t pages, uint32_t crc);
void firmware_page_received(uint8_t id, uint16_t page, uint8_t *data);
uint16_t firmware_get_version(void);
uint16_t firmware_get_pages(void);
uint32_t firmware_get_checksum(void);
void firmware_get_page(uint16_t page, uint8_t *data);

#endif	/* FIRMWARE_H */

