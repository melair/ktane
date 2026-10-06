#ifndef BACKPLANE_BOARD_IDENTITY_H
#define BACKPLANE_BOARD_IDENTITY_H

#include "version/version.h"

/* Shared by this board's application and bootloader descriptors. */
#define BOARD_PLATFORM VERSION_PLATFORM_BACKPLANE
#define BOARD_HARDWARE VERSION_HARDWARE('B', 1)

#endif /* BACKPLANE_BOARD_IDENTITY_H */
