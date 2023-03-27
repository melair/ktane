#ifndef PN532_H
#define	PN532_H

#include <stdbool.h>

void pn532_service(void);
void pn532_service_samconfigure_callback(bool ok);
void pn532_service_inpassivetarget_callback(bool ok);
void pn532_service_detect_callback(bool ok);
void pn532_service_mfu_read_callback(bool ok);
void pn532_service_mfu_write_callback(bool ok);

#endif	/* PN532_H */

