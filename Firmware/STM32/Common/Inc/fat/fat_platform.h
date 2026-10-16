#ifndef FAT_PLATFORM_H
#define FAT_PLATFORM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "block_io/block_io.h"

/* One backend supplied by the consuming board. It must honor the BlockIO
 * queue/follow-on contract; FAT uses follow-ons to serialize metadata changes.
 * Service it before FAT_Service() every main-loop pass, so readiness loss is
 * observed before another medium becomes available. Hooks never service I/O.
 * queue only enqueues the transaction; it must not access its data buffer or
 * start I/O until a later backend service pass. FAT finishes initializing the
 * request and buffer after queue accepts, before submission returns.
 */
typedef struct {
    bool (*queue)(BlockIO_Transaction *transaction);
    uint32_t (*sector_count)(void); /* Zero unless media is ready and present. */
    bool (*buffer_valid)(const void *data, size_t size);
    uint32_t (*get_tick)(void); /* Wrapping monotonic millisecond counter. */
} FAT_Platform;

extern const FAT_Platform FAT_PLATFORM;

#endif // FAT_PLATFORM_H
