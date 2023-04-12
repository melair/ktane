#ifndef FW_SERVER_H
#define	FW_SERVER_H

void fw_server_recieve_header_request(uint8_t id, packet_t *p);
void fw_server_recieve_page_request(uint8_t id, packet_t *p);

#endif	/* FW_SERVER_H */

