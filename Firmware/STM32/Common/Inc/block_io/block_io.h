#ifndef BLOCK_IO_H
#define BLOCK_IO_H

#include <stdint.h>

#define BLOCK_IO_SECTOR_SIZE 512u

/* Main-loop-only asynchronous sector I/O. Submission must reject without
 * mutation or callbacks, or set PENDING and complete on a later service pass.
 * Keep transactions and buffers alive and unmodified while pending. Backends
 * define buffer restrictions; writes do not modify data, failed reads may be
 * partial. Release buffer ownership before delivering completion.
 *
 * Queue entries run FIFO. Completion callbacks may return a follow-on (including
 * the same transaction), which runs before queued work. Never also queue that
 * object. Invalid follow-ons complete as INVALID on a later pass. Media loss
 * cancels outstanding work after releasing buffers and ignores follow-ons;
 * ignored follow-ons retain their previous status. NULL callbacks allow polling.
 * Do not recursively service a backend from its callbacks.
 */
typedef enum {
    BLOCK_IO_STATUS_SUCCESS,
    BLOCK_IO_STATUS_UNAVAILABLE,
    BLOCK_IO_STATUS_CANCELLED,
    BLOCK_IO_STATUS_TIMEOUT,
    BLOCK_IO_STATUS_TRANSFER_ERROR,
    BLOCK_IO_STATUS_PENDING,
    BLOCK_IO_STATUS_INVALID,
} BlockIO_Status;

typedef enum {
    BLOCK_IO_OPERATION_READ,
    BLOCK_IO_OPERATION_WRITE,
} BlockIO_Operation;

typedef struct BlockIO_Transaction BlockIO_Transaction;

struct BlockIO_Transaction {
    BlockIO_Operation operation;
    volatile BlockIO_Status status;
    uint32_t sector;
    uint32_t sector_count;
    void *data;
    BlockIO_Transaction *(*callback)(BlockIO_Transaction *);
    void *callback_data;
    BlockIO_Transaction *queue_next;
};

#endif // BLOCK_IO_H
