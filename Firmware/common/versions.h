#ifndef VERSIONS_H
#define	VERSIONS_H

#include <stdint.h>

#define BOOTLOADER_VERSION  0
#define APPLICATION_VERSION 1
#define FLASHER_VERSION     2

void versions_initialise(void);
uint16_t versions_get(uint8_t v);

#endif	/* VERSIONS_H */

