#ifndef NVM_INTERNAL_H
#define NVM_INTERNAL_H

#include <stdbool.h>

/* Called by the MCU NMI handler to recover from an ECC error during an NVM read. */
bool NVM_HandleNMI(void);

#endif // NVM_INTERNAL_H
