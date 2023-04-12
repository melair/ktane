#ifndef FW_UPDATER_H
#define	FW_UPDATER_H

#include "packet.h"

void fw_updater_receive_header(uint8_t id, packet_t *p);
void fw_updater_receive_page(uint8_t id, packet_t *p);
void fw_updater_start(uint8_t segment, uint16_t version);

#endif	/* FW_UPDATER_H */

