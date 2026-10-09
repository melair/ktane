#ifndef FAT_H
#define FAT_H

#include <stddef.h>
#include <stdint.h>
#include "sys/sdcard.h"

#define FAT_SECTOR_SIZE 512u
#define FAT_ROOT_SECTOR 0u
#define FAT_MAX_ENTRIES 21u
#define FAT_NAME_SIZE 10u
#define FAT_LAST_SECTOR 0xFFFFFFFF
#define FAT_MAGIC 0x46415401
#define FAT_MAX_FILE_SIZE (UINT32_C(65535) * FAT_SECTOR_SIZE)

#ifndef FAT_CACHE_CAPACITY
#define FAT_CACHE_CAPACITY 32u
#endif

#pragma pack(push, 1)

typedef struct {
    uint32_t magic;
    uint32_t next_fat_sector; /* FAT_LAST_SECTOR terminates the chain. */
} FAT_Header;

typedef struct {
    uint32_t starting_sector; /* FAT_LAST_SECTOR indicates entry not in use. */
    uint16_t sector_count;
    struct {
        unsigned last_sector_bytes_count : 10;
        unsigned present : 1;
        unsigned reserved : 5;
    } compacted;
    uint8_t mode_id;
    uint8_t type_id;
    uint8_t meta[4]; /* Interpretation depends on type_id. */
    char name[FAT_NAME_SIZE]; /* NUL padded; all 10 bytes may be used. */
} FAT_Entry;

typedef struct {
    FAT_Header header;
    FAT_Entry entries[FAT_MAX_ENTRIES];
} FAT_Sector;

#pragma pack(pop)

/* Runtime types below are naturally aligned, rather than packed. */
typedef struct {
    uint8_t mode_id;
    uint8_t type_id;
    char name[FAT_NAME_SIZE]; /* Exact 10-byte key, NUL padded if shorter. */
} FAT_Key;

typedef struct {
    FAT_Key key;
    uint32_t starting_sector;
    uint16_t sector_count;
    uint16_t last_sector_bytes_count;
    uint32_t size_bytes;
    uint8_t meta[4];
} FAT_File;

typedef enum {
    FAT_OPERATION_FIND,
    FAT_OPERATION_CREATE,
    FAT_OPERATION_FORMAT,
} FAT_Operation;

typedef enum {
    FAT_STATUS_PENDING,
    FAT_STATUS_SUCCESS,
    FAT_STATUS_NOT_FOUND,
    FAT_STATUS_INVALID_ARGUMENT,
    FAT_STATUS_UNAVAILABLE,
    FAT_STATUS_BUSY,
    FAT_STATUS_INVALID_FORMAT,
    FAT_STATUS_CORRUPT_FAT,
    FAT_STATUS_NO_SPACE,
    FAT_STATUS_TIMEOUT,
    FAT_STATUS_IO_ERROR,
} FAT_Status;

typedef struct FAT_Request FAT_Request;

/* Request-owned SRAM buffer. The words member ensures the four-byte alignment
 * required by SDCard_Queue(), while sector exposes the packed disk layout.
 */
typedef union {
    FAT_Sector sector;
    uint32_t words[FAT_SECTOR_SIZE / sizeof(uint32_t)];
} FAT_SectorBuffer;

/* Library-owned continuation state: callers must not read or modify it.
 * Every I/O request owns its SD transaction and buffer, allowing SDCard's
 * queue and callback follow-ons to serialize FAT work without a FAT I/O queue.
 */
typedef struct {
    SDCard_Transaction sd_transaction;
    FAT_SectorBuffer buffer;
    FAT_Request *completion_next; /* Deferred cache-hit delivery only. */
    uint32_t media_generation;
    uint32_t scan_sector;
    uint32_t tail_sector;
    uint32_t high_water_sector;
    uint32_t match_sector;
    uint32_t free_entry_sector;
    uint32_t new_fat_sector;
    uint32_t new_data_sector;
    uint32_t traversal_remaining;
    uint16_t match_index;
    uint16_t free_entry_index;
    uint8_t phase;
} FAT_RequestInternal;

struct FAT_Request {
    FAT_Operation operation; /* Assigned by the submission function. */
    volatile FAT_Status status;
    FAT_Key key;              /* Input to find/create; ignored by format. */
    uint32_t size_bytes;       /* Create input, including zero-byte files. */
    uint8_t meta[4];           /* Create input; not part of the lookup key. */
    FAT_File result;           /* Valid only after successful find/create. */
    void (*callback)(FAT_Request *request);
    void *callback_data;
    FAT_RequestInternal internal;
};

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize once before submitting requests. Does not initialize SDCard,
 * access the card, or format it. Do not reinitialize with outstanding work.
 * All FAT APIs and callbacks are main-loop only.
 */
void FAT_Init(void);

/* Call after SDCard_Service() on every main-loop pass. Delivers deferred
 * cache-hit completions and observes card readiness. I/O callbacks are
 * delivered by SDCard_Service(). Never invoke either service recursively.
 * A callback may submit another request, or resubmit its completed request;
 * cache-hit requests submitted there are delivered on a later service pass.
 */
void FAT_Service(void);

/* Submission contract shared by find/create/format:
 * - Initialize the request (e.g. to zero), callback and callback_data, plus
 *   the operation's input fields. Names may use all 10 bytes; pad shorter
 *   names with NULs. Matching compares mode_id, type_id and all name bytes.
 * - PENDING accepts the request, assigns operation/status, and guarantees
 *   exactly one deferred completion. Callbacks never run during submission,
 *   even on cache hits. NULL callbacks allow polling status instead.
 * - Any other return value rejects submission without changing the request
 *   or invoking its callback. Reject invalid arguments and unavailable cards;
 *   BUSY also covers duplicate submission and formatting exclusivity.
 * - Keep the request in SD-accessible SRAM, alive and unmodified until its
 *   status leaves PENDING. Never submit one request twice while pending.
 * - Successful results are copied values, independent of cache eviction.
 *   Results are invalid after card removal, formatting or replacement of
 *   that key. Callers perform file payload I/O directly through SDCard.
 * - Loss of readiness clears the cache and allocation watermark, and completes
 *   outstanding requests as UNAVAILABLE (or TIMEOUT/IO_ERROR for the failing
 *   I/O). Stale media generations must never return success or repopulate the cache.
 *
 * The fixed cache stores successful find/create results in stationary slots.
 * Each access records HAL_GetTick(); eviction selects the greatest unsigned
 * elapsed age, breaking equal-age ties by slot index. Cache hits never need
 * SD I/O. Only their deferred completions use a FAT-owned intrusive list.
 */

/* Find the first present entry, using the cache first, then reading the FAT
 * chain from sector 0. Stop at a match; reaching the chain end without one
 * completes as NOT_FOUND and establishes the allocation watermark.
 * FAT_LAST_SECTOR starting_sector marks unused entries; other entries reserve
 * their extents even if present is zero, but find skips non-present entries.
 * Missing root FAT_MAGIC reports INVALID_FORMAT. Unusable entry sizes, links
 * or sector bounds, or exhausting the traversal limit, report CORRUPT_FAT.
 * Traversal is bounded by the card's logical sector count. No overlap or
 * duplicate-key integrity checks are performed. Never format implicitly.
 */
FAT_Status FAT_Find(FAT_Request *request);

/* Create or replace the entry identified by key, with size_bytes and meta.
 * Scan for the first matching entry, retaining the first unused entry as a
 * fallback. Complete the scan if the highest occupied FAT/data sector is not
 * yet known, or if no match is found. Once a complete scan establishes that
 * watermark, replacements can stop scanning at their matching entry.
 * Replacement reuses the matching entry's starting sector if the new file
 * fits in its allocated sectors, or if its nonempty extent ends
 * at the highest occupied sector (allowing growth up to card capacity).
 * Otherwise append contiguous data after the highest occupied sector. Interior
 * gaps are not searched. Shrinking records only the new extent; space released
 * at the end is available to later allocations, including after a reboot.
 * Successful writes update the known watermark; shrinking or emptying its
 * highest extent invalidates it so the next create can recalculate it. The FAT
 * chain tail and the allocation watermark are separate sector addresses.
 *
 * If no entry is available, append a FAT sector first, placing new data after
 * it. Initialize unused starting_sector fields to FAT_LAST_SECTOR; write the
 * new FAT sector before linking it from the previous tail. No other FAT I/O
 * operation may interleave this scan/write sequence: use SD callback follow-ons.
 * Success means all required FAT writes completed and present is set to one;
 * it does not mean the caller has written the file payload.
 *
 * size_bytes must be <= FAT_MAX_FILE_SIZE. For nonempty files, sector_count
 * is ceil(size_bytes / 512), and last_sector_bytes_count is 1..512 (512 means
 * a full final sector). Empty files use starting_sector=0, sector_count=0,
 * last_sector_bytes_count=0; callers must skip payload SD I/O. Clear reserved
 * bits when writing entries. Insufficient card capacity completes as NO_SPACE.
 * Emptying a file releases its extent without allocating another FAT sector.
 *
 * Interrupted/failed writes may leave uncertain on-card state; there is no
 * transactional recovery guarantee. Discard cached results after write failure
 * and re-read the FAT before attempting further allocation.
 */
FAT_Status FAT_Create(FAT_Request *request);

/* Explicitly reset sector 0 to an empty FAT: FAT_MAGIC, next_fat_sector set to
 * FAT_LAST_SECTOR, and all entries unused. Clear the cache when accepted, and
 * report SUCCESS only after the root write completes. Payload sectors are not
 * erased, but their old allocations are forgotten and may be overwritten.
 * Reject as BUSY if any FAT request is outstanding; reject find/create while
 * formatting. The caller must also finish external SD I/O and stop using old
 * file locations before formatting. key/size_bytes/meta are ignored.
 */
FAT_Status FAT_Format(FAT_Request *request);

#ifdef __cplusplus
}
#endif

#endif /* FAT_H */
