#ifndef COMMS_BOARD_IDENTITY_H
#define COMMS_BOARD_IDENTITY_H

#include "version/version.h"

/* Shared by this board's application and bootloader descriptors. */
#define BOARD_PLATFORM VERSION_PLATFORM_COMMS
#define BOARD_HARDWARE VERSION_HARDWARE('C', 1)

#endif /* COMMS_BOARD_IDENTITY_H */
