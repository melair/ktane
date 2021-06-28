#ifndef PROTOCOL_FIRMWARE_H
#define	PROTOCOL_FIRMWARE_H

void protocol_firmware_receive(uint8_t id, uint8_t size, uint8_t *payload);
void protocol_firmware_request_send(uint16_t requested_version);
void protocol_firmware_page_request_send(uint16_t page, uint8_t source_id);

#endif	/* PROTOCOL_FIRMWARE_H */

