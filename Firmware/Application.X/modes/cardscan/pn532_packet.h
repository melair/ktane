#ifndef PN532_PACKET_H
#define	PN532_PACKET_H

#include <stdint.h>

uint8_t *pn532_packet_prepare(uint8_t *buff, uint8_t cmd_len);
void pn532_packet_finalise(uint8_t *buff);
uint8_t pn532_packet_size(uint8_t cmd_len);

uint8_t pn532_getfirmwareversion(uint8_t *buff);
uint8_t pn532_samconfigure(uint8_t *buff);
uint8_t pn532_inlistpassivetarget(uint8_t *buff);
uint8_t pn532_statread(uint8_t *buff);
uint8_t pn532_dataread(uint8_t *buff);
uint8_t pn532_mifareultralight_write(uint8_t *buff, uint8_t page, uint8_t *data);
uint8_t pn532_mifareultralight_read(uint8_t *buff, uint8_t page);
        
#endif	/* PN532_PACKET_H */

