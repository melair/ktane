#ifndef DEBUG_CONFIG_H
#define DEBUG_CONFIG_H

/* The BLE stack's low-power interface references RTDebug unconditionally.
 * Keep its project configuration disabled without bringing in GPIO support. */
#define RT_DEBUG_GPIO_MODULE (0)

#include "debug_signals.h"

#endif /* DEBUG_CONFIG_H */
