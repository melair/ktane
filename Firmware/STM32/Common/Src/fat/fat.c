#include "fat/fat.h"
#include "fat/fat_platform.h"

#include <stdbool.h>
#include <string.h>

_Static_assert(sizeof(FAT_Type) == sizeof(uint8_t), "FAT type must occupy one byte");
_Static_assert(sizeof(FAT_Header) == 8, "FAT header layout changed");
_Static_assert(sizeof(FAT_Entry) == 24, "FAT entry layout changed");
_Static_assert(sizeof(FAT_Sector) == FAT_SECTOR_SIZE, "FAT sector layout changed");
_Static_assert(_Alignof(FAT_SectorBuffer) >= 4, "Block I/O buffer must be aligned");
_Static_assert(FAT_CACHE_CAPACITY > 0, "FAT cache must have entries");

enum {
    PHASE_IDLE, PHASE_SCAN, PHASE_TARGET_READ, PHASE_NEW_WRITE,
    PHASE_TAIL_READ, PHASE_COMMIT_WRITE, PHASE_CACHED,
};

typedef struct {
    bool valid;
    uint32_t tick;
    FAT_File file;
} CacheEntry;

typedef struct {
    uint32_t data_start_sector;
    uint32_t fat_sector;
    uint32_t entry_sector;
    uint16_t entry_index;
    bool needs_fat_sector;
    bool releases_tail;
} AllocationPlan;

static struct {
    bool initialized;
    bool ready;
    bool formatting;
    uint32_t generation;
    uint32_t outstanding;
    CacheEntry cache[FAT_CACHE_CAPACITY];
    FAT_Request *completion_head;
    FAT_Request *completion_tail;
    FAT_Request *continuation; /* Active follow-on, not an I/O work queue. */
    uint32_t high_water_sector;
    bool watermark_complete;
    /* Block I/O follow-ons serialize I/O, so only one allocation plan is active. */
    AllocationPlan allocation;
} fat;

static BlockIO_Transaction *io_complete(BlockIO_Transaction *transaction);

static void invalidate(void) {
    memset(fat.cache, 0, sizeof(fat.cache));
    fat.watermark_complete = false;
    ++fat.generation;
}

static uint32_t media_capacity(void) {
    const uint32_t capacity = FAT_PLATFORM.sector_count();
    const bool ready = capacity != 0;
    if (fat.ready && !ready) {
        invalidate();
    }
    fat.ready = ready;
    return capacity;
}

static bool key_equal(const FAT_Key *a, const FAT_Key *b) {
    return a->mode_id == b->mode_id && a->type_id == b->type_id &&
        memcmp(a->name, b->name, FAT_NAME_SIZE) == 0;
}

static void file_from_entry(const FAT_Entry *entry, FAT_File *file) {
    memset(file, 0, sizeof(*file));
    file->key.mode_id = entry->mode_id;
    file->key.type_id = entry->type_id;
    memcpy(file->key.name, entry->name, FAT_NAME_SIZE);
    file->starting_sector = entry->starting_sector;
    file->sector_count = entry->sector_count;
    file->last_sector_bytes_count = entry->compacted.last_sector_bytes_count;
    file->size_bytes = entry->sector_count == 0 ? 0 :
        ((uint32_t)entry->sector_count - 1u) * FAT_SECTOR_SIZE +
        entry->compacted.last_sector_bytes_count;
    memcpy(file->meta, entry->meta, sizeof(file->meta));
}

static CacheEntry *cache_find(const FAT_Key *key) {
    for (size_t i = 0; i < FAT_CACHE_CAPACITY; ++i) {
        if (fat.cache[i].valid && key_equal(key, &fat.cache[i].file.key)) {
            return &fat.cache[i];
        }
    }
    return NULL;
}

static void cache_store(const FAT_File *file) {
    const uint32_t now = FAT_PLATFORM.get_tick();
    CacheEntry *slot = cache_find(&file->key);
    if (slot == NULL) {
        slot = &fat.cache[0];
        for (size_t i = 0; i < FAT_CACHE_CAPACITY; ++i) {
            if (!fat.cache[i].valid) {
                slot = &fat.cache[i];
                break;
            }
            if ((uint32_t)(now - fat.cache[i].tick) >
                (uint32_t)(now - slot->tick)) {
                slot = &fat.cache[i];
            }
        }
    }
    slot->valid = true;
    slot->tick = now;
    slot->file = *file;
}

static void publish_result(const FAT_Request *request) {
    cache_store(&request->result);
    /* A replacement must also refresh already deferred cache-hit results. */
    if (request->operation == FAT_OPERATION_CREATE) {
        for (FAT_Request *hit = fat.completion_head; hit != NULL;
             hit = hit->internal.completion_next) {
            if (hit->internal.media_generation == fat.generation &&
                key_equal(&hit->key, &request->key)) {
                hit->result = request->result;
            }
        }
    }
}

static void commit_watermark(const FAT_Request *request) {
    if (request->operation == FAT_OPERATION_FORMAT) {
        fat.high_water_sector = FAT_ROOT_SECTOR;
        fat.watermark_complete = true;
    } else if (request->operation == FAT_OPERATION_CREATE) {
        if (fat.allocation.releases_tail) {
            /* The next complete scan will establish the lower endpoint. */
            fat.watermark_complete = false;
            return;
        }
        if (fat.allocation.needs_fat_sector) {
            fat.high_water_sector = fat.allocation.fat_sector;
        }
        if (request->result.sector_count != 0) {
            const uint32_t data_end = request->result.starting_sector +
                                      request->result.sector_count - 1u;
            if (data_end > fat.high_water_sector) {
                fat.high_water_sector = data_end;
            }
        }
    }
}

static BlockIO_Transaction *finish(FAT_Request *request, FAT_Status status) {
    if (status == FAT_STATUS_SUCCESS) {
        commit_watermark(request);
        if (request->operation != FAT_OPERATION_FORMAT) {
            publish_result(request);
        }
    }
    /* Release all ownership before calling code that may reuse the request. */
    if (request->operation == FAT_OPERATION_FORMAT) {
        fat.formatting = false;
    }
    if (fat.continuation == request) {
        fat.continuation = NULL;
    }
    --fat.outstanding;
    request->internal.phase = PHASE_IDLE;
    request->internal.completion_next = NULL;
    request->status = status;
    void (*callback)(FAT_Request *) = request->callback;
    if (callback != NULL) {
        callback(request);
    }
    /* The callback may have resubmitted or released request. */
    return NULL;
}

static BlockIO_Transaction *transfer(FAT_Request *request, uint32_t sector,
                                    BlockIO_Operation operation, uint8_t phase) {
    request->internal.phase = phase;
    BlockIO_Transaction *transaction = &request->internal.transaction;
    transaction->operation = operation;
    transaction->sector = sector;
    transaction->sector_count = 1;
    transaction->data = request->internal.buffer.words;
    transaction->callback = io_complete;
    transaction->callback_data = request;
    return transaction;
}

static bool entry_allocated(const FAT_Entry *entry) {
    return entry->starting_sector != FAT_LAST_SECTOR;
}

static bool valid_sector(const FAT_Sector *sector, uint32_t address, uint32_t capacity) {
    const uint32_t next = sector->header.next_fat_sector;
    if (sector->header.magic != FAT_MAGIC ||
        (next != FAT_LAST_SECTOR && (next >= capacity || next == address))) {
        return false;
    }
    for (size_t i = 0; i < FAT_MAX_ENTRIES; ++i) {
        const FAT_Entry *entry = &sector->entries[i];
        if (!entry_allocated(entry)) {
            continue;
        }
        if (entry->sector_count == 0) {
            if (entry->starting_sector != 0 ||
                entry->compacted.last_sector_bytes_count != 0) {
                return false;
            }
        } else if (entry->compacted.last_sector_bytes_count == 0 ||
                   entry->compacted.last_sector_bytes_count > FAT_SECTOR_SIZE ||
                   entry->starting_sector >= capacity ||
                   entry->sector_count > capacity - entry->starting_sector) {
            return false;
        }
    }
    return true;
}

static void empty_sector(FAT_Sector *sector) {
    memset(sector, 0, sizeof(*sector));
    sector->header.magic = FAT_MAGIC;
    sector->header.next_fat_sector = FAT_LAST_SECTOR;
    for (size_t i = 0; i < FAT_MAX_ENTRIES; ++i) {
        sector->entries[i].starting_sector = FAT_LAST_SECTOR;
    }
}

static void create_entry(FAT_Request *request, uint16_t index) {
    FAT_Entry *entry = &request->internal.buffer.sector.entries[index];
    memset(entry, 0, sizeof(*entry));
    entry->starting_sector = request->internal.new_data_sector;
    entry->sector_count = (uint16_t)((request->size_bytes + FAT_SECTOR_SIZE - 1u) /
                                    FAT_SECTOR_SIZE);
    entry->compacted.last_sector_bytes_count = request->size_bytes == 0 ? 0 :
        ((request->size_bytes - 1u) % FAT_SECTOR_SIZE) + 1u;
    entry->compacted.present = 1;
    entry->mode_id = request->key.mode_id;
    entry->type_id = request->key.type_id;
    memcpy(entry->name, request->key.name, FAT_NAME_SIZE);
    memcpy(entry->meta, request->meta, sizeof(entry->meta));
    file_from_entry(entry, &request->result);
}

static bool plan_allocation(const FAT_Request *request, uint32_t capacity,
                            AllocationPlan *plan) {
    const FAT_RequestInternal *state = &request->internal;
    const bool replacing = state->match_sector != FAT_LAST_SECTOR;
    const uint32_t required_sectors =
        (request->size_bytes + FAT_SECTOR_SIZE - 1u) / FAT_SECTOR_SIZE;
    const uint64_t append_sector = (uint64_t)fat.high_water_sector + 1u;
    const bool at_tail = replacing && request->result.sector_count != 0 &&
        request->result.starting_sector + request->result.sector_count - 1u ==
        fat.high_water_sector;
    plan->needs_fat_sector = !replacing && state->free_entry_sector == FAT_LAST_SECTOR;
    plan->releases_tail = at_tail && required_sectors < request->result.sector_count;
    uint64_t data_start_sector = append_sector + (plan->needs_fat_sector ? 1u : 0u);
    if (replacing && (required_sectors <= request->result.sector_count || at_tail)) {
        data_start_sector = request->result.starting_sector;
    }
    if ((plan->needs_fat_sector &&
         (append_sector >= capacity || append_sector >= FAT_LAST_SECTOR)) ||
        (required_sectors != 0 && (data_start_sector >= FAT_LAST_SECTOR ||
                                  data_start_sector + required_sectors > capacity))) {
        return false;
    }
    plan->data_start_sector = required_sectors == 0 ? 0 : (uint32_t)data_start_sector;
    plan->fat_sector = plan->needs_fat_sector ? (uint32_t)append_sector : FAT_LAST_SECTOR;
    plan->entry_sector = plan->needs_fat_sector ? plan->fat_sector :
        (replacing ? state->match_sector : state->free_entry_sector);
    plan->entry_index = plan->needs_fat_sector ? 0 :
        (replacing ? state->match_index : state->free_entry_index);
    return true;
}

static BlockIO_Transaction *write_entry(FAT_Request *request) {
    create_entry(request, fat.allocation.entry_index);
    return transfer(request, fat.allocation.entry_sector, BLOCK_IO_OPERATION_WRITE,
                    fat.allocation.needs_fat_sector ? PHASE_NEW_WRITE : PHASE_COMMIT_WRITE);
}

static BlockIO_Transaction *allocate(FAT_Request *request, uint32_t capacity) {
    AllocationPlan plan;
    if (!plan_allocation(request, capacity, &plan)) {
        return finish(request, FAT_STATUS_NO_SPACE);
    }
    fat.allocation = plan;
    FAT_RequestInternal *state = &request->internal;
    state->new_data_sector = plan.data_start_sector;
    state->new_fat_sector = plan.fat_sector;
    if (plan.needs_fat_sector) {
        empty_sector(&state->buffer.sector);
        return write_entry(request);
    }
    if (state->scan_sector == plan.entry_sector) {
        return write_entry(request);
    }
    return transfer(request, plan.entry_sector, BLOCK_IO_OPERATION_READ, PHASE_TARGET_READ);
}

/* Each block is read once. A NULL query only gathers allocation information;
 * an existing match is retained while the remainder establishes the watermark. */
static BlockIO_Transaction *scan_fat(FAT_Request *request, const FAT_Key *query,
                                  uint32_t capacity) {
    FAT_RequestInternal *state = &request->internal;
    const FAT_Sector *sector = &state->buffer.sector;
    if (sector->header.magic != FAT_MAGIC) {
        fat.watermark_complete = false;
        return finish(request, state->scan_sector == FAT_ROOT_SECTOR ?
                      FAT_STATUS_INVALID_FORMAT : FAT_STATUS_CORRUPT_FAT);
    }
    if (state->traversal_remaining == 0 ||
        !valid_sector(sector, state->scan_sector, capacity)) {
        fat.watermark_complete = false;
        return finish(request, FAT_STATUS_CORRUPT_FAT);
    }
    --state->traversal_remaining;
    if (state->scan_sector > state->high_water_sector) {
        state->high_water_sector = state->scan_sector;
    }
    for (uint16_t i = 0; i < FAT_MAX_ENTRIES; ++i) {
        const FAT_Entry *entry = &sector->entries[i];
        if (!entry_allocated(entry)) {
            if (state->free_entry_sector == FAT_LAST_SECTOR) {
                state->free_entry_sector = state->scan_sector;
                state->free_entry_index = i;
            }
            continue;
        }
        if (entry->sector_count != 0) {
            const uint32_t data_end = entry->starting_sector + entry->sector_count - 1u;
            if (data_end > state->high_water_sector) {
                state->high_water_sector = data_end;
            }
        }
        if (query != NULL && state->match_sector == FAT_LAST_SECTOR &&
            (entry->compacted.present || request->operation == FAT_OPERATION_CREATE) &&
            entry->mode_id == query->mode_id && entry->type_id == query->type_id &&
            memcmp(entry->name, query->name, FAT_NAME_SIZE) == 0) {
            state->match_sector = state->scan_sector;
            state->match_index = i;
            file_from_entry(entry, &request->result);
        }
    }
    state->tail_sector = state->scan_sector;
    const uint32_t next = sector->header.next_fat_sector;
    const bool scan_complete = next == FAT_LAST_SECTOR;
    if (scan_complete) {
        fat.high_water_sector = state->high_water_sector;
        fat.watermark_complete = true;
    }
    const bool matched = state->match_sector != FAT_LAST_SECTOR;
    if (request->operation == FAT_OPERATION_FIND && (matched || scan_complete)) {
        return finish(request, matched ? FAT_STATUS_SUCCESS : FAT_STATUS_NOT_FOUND);
    }
    if (scan_complete || (matched && fat.watermark_complete)) {
        return allocate(request, capacity);
    }
    state->scan_sector = next;
    return transfer(request, next, BLOCK_IO_OPERATION_READ, PHASE_SCAN);
}

static BlockIO_Transaction *io_complete(BlockIO_Transaction *transaction) {
    FAT_Request *request = transaction->callback_data;
    FAT_RequestInternal *state = &request->internal;
    const BlockIO_Status status = transaction->status;
    const uint32_t capacity = media_capacity();
    const bool ready = capacity != 0;
    if (status != BLOCK_IO_STATUS_SUCCESS) {
        if (transaction->operation == BLOCK_IO_OPERATION_WRITE ||
            state->phase == PHASE_TAIL_READ) {
            invalidate();
        }
        FAT_Status failure = FAT_STATUS_IO_ERROR;
        if (status == BLOCK_IO_STATUS_TIMEOUT) {
            failure = FAT_STATUS_TIMEOUT;
        } else if (!ready || status == BLOCK_IO_STATUS_UNAVAILABLE ||
                   status == BLOCK_IO_STATUS_CANCELLED) {
            failure = FAT_STATUS_UNAVAILABLE;
        }
        return finish(request, failure);
    }
    if (!ready || state->media_generation != fat.generation) {
        return finish(request, FAT_STATUS_UNAVAILABLE);
    }
    fat.continuation = request;
    FAT_Sector *sector = &state->buffer.sector;
    switch (state->phase) {
        case PHASE_SCAN:
            return scan_fat(request, state->match_sector == FAT_LAST_SECTOR ?
                            &request->key : NULL, capacity);
        case PHASE_TARGET_READ:
            if (!valid_sector(sector, transaction->sector, capacity)) {
                return finish(request, FAT_STATUS_CORRUPT_FAT);
            }
            return write_entry(request);
        case PHASE_NEW_WRITE:
            return transfer(request, state->tail_sector, BLOCK_IO_OPERATION_READ, PHASE_TAIL_READ);
        case PHASE_TAIL_READ:
            if (!valid_sector(sector, state->tail_sector, capacity) ||
                sector->header.next_fat_sector != FAT_LAST_SECTOR) {
                invalidate(); /* A new block was written but was not linked. */
                return finish(request, FAT_STATUS_CORRUPT_FAT);
            }
            sector->header.next_fat_sector = state->new_fat_sector;
            return transfer(request, state->tail_sector, BLOCK_IO_OPERATION_WRITE, PHASE_COMMIT_WRITE);
        case PHASE_COMMIT_WRITE:
            return finish(request, FAT_STATUS_SUCCESS);
        default:
            return finish(request, FAT_STATUS_IO_ERROR);
    }
}

void FAT_Init(void) {
    if (fat.outstanding != 0) {
        return;
    }
    memset(&fat, 0, sizeof(fat));
    fat.initialized = true;
    fat.generation = 1;
}

void FAT_Service(void) {
    if (!fat.initialized) {
        return;
    }
    const bool ready = media_capacity() != 0;
    /* Detach this batch so callbacks cannot cause delivery of newly submitted
     * cache hits within the same service pass. */
    FAT_Request *request = fat.completion_head;
    fat.completion_head = NULL;
    fat.completion_tail = NULL;
    /* Removal can occur between our callback returning a follow-on and the
     * backend's readiness check. It then discards the follow-on, leaving
     * its transaction SUCCESS rather than PENDING and no backend queue reference.
     * Complete that orphan here; never release a still-pending I/O buffer. */
    if (fat.continuation != NULL &&
        fat.continuation->internal.transaction.status != BLOCK_IO_STATUS_PENDING) {
        FAT_Request *orphan = fat.continuation;
        invalidate();
        finish(orphan, FAT_STATUS_UNAVAILABLE);
    }
    while (request != NULL) {
        FAT_Request *next = request->internal.completion_next;
        finish(request, ready && request->internal.media_generation == fat.generation &&
               media_capacity() != 0 ? FAT_STATUS_SUCCESS : FAT_STATUS_UNAVAILABLE);
        request = next;
    }
}

static FAT_Status submit(FAT_Request *request, FAT_Operation operation) {
    const uintptr_t address = (uintptr_t)request;
    if (request == NULL || address % _Alignof(FAT_Request) != 0 ||
        !FAT_PLATFORM.buffer_valid(request->internal.buffer.words, FAT_SECTOR_SIZE) ||
        (operation == FAT_OPERATION_CREATE && request->size_bytes > FAT_MAX_FILE_SIZE)) {
        return FAT_STATUS_INVALID_ARGUMENT;
    }
    if (request->internal.phase != PHASE_IDLE || fat.formatting ||
        (operation == FAT_OPERATION_FORMAT && fat.outstanding != 0)) {
        return FAT_STATUS_BUSY;
    }
    if (!fat.initialized) {
        return FAT_STATUS_UNAVAILABLE;
    }
    const uint32_t capacity = media_capacity();
    if (capacity == 0) {
        return FAT_STATUS_UNAVAILABLE;
    }
    if (operation == FAT_OPERATION_FIND) {
        CacheEntry *hit = cache_find(&request->key);
        if (hit != NULL) {
            hit->tick = FAT_PLATFORM.get_tick();
            request->operation = operation;
            request->status = FAT_STATUS_PENDING;
            request->result = hit->file;
            memset(&request->internal, 0, sizeof(request->internal));
            request->internal.phase = PHASE_CACHED;
            request->internal.media_generation = fat.generation;
            if (fat.completion_tail != NULL) {
                fat.completion_tail->internal.completion_next = request;
            } else {
                fat.completion_head = request;
            }
            fat.completion_tail = request;
            ++fat.outstanding;
            return FAT_STATUS_PENDING;
        }
    }
    /* Queue before initializing the request or its sector buffer. The backend
     * only accesses the buffer on a later service pass, so rejection needs to
     * restore just the transaction descriptor, not a sector-sized request. */
    BlockIO_Transaction *transaction = &request->internal.transaction;
    const BlockIO_Transaction saved = *transaction;
    *transaction = (BlockIO_Transaction) {
        .operation = operation == FAT_OPERATION_FORMAT
            ? BLOCK_IO_OPERATION_WRITE : BLOCK_IO_OPERATION_READ,
        .sector = FAT_ROOT_SECTOR,
        .sector_count = 1,
        .data = request->internal.buffer.words,
        .callback = io_complete,
        .callback_data = request,
    };
    if (!FAT_PLATFORM.queue(transaction)) {
        *transaction = saved;
        return media_capacity() != 0 ? FAT_STATUS_INVALID_ARGUMENT : FAT_STATUS_UNAVAILABLE;
    }

    /* Preserve the accepted transaction, including backend queue links. */
    memset(&request->internal.buffer, 0,
           sizeof(request->internal) - offsetof(FAT_RequestInternal, buffer));
    request->internal.media_generation = fat.generation;
    request->internal.match_sector = FAT_LAST_SECTOR;
    request->internal.free_entry_sector = FAT_LAST_SECTOR;
    request->internal.new_fat_sector = FAT_LAST_SECTOR;
    request->internal.traversal_remaining = capacity;
    request->operation = operation;
    request->status = FAT_STATUS_PENDING;
    memset(&request->result, 0, sizeof(request->result));
    if (operation == FAT_OPERATION_FORMAT) {
        empty_sector(&request->internal.buffer.sector);
        request->internal.phase = PHASE_COMMIT_WRITE;
    } else {
        request->internal.phase = PHASE_SCAN;
    }
    ++fat.outstanding;
    if (operation == FAT_OPERATION_FORMAT) {
        invalidate();
        request->internal.media_generation = fat.generation;
        fat.formatting = true;
    }
    return FAT_STATUS_PENDING;
}

FAT_Status FAT_Find(FAT_Request *request) {
    return submit(request, FAT_OPERATION_FIND);
}

FAT_Status FAT_Create(FAT_Request *request) {
    return submit(request, FAT_OPERATION_CREATE);
}

FAT_Status FAT_Format(FAT_Request *request) {
    return submit(request, FAT_OPERATION_FORMAT);
}
