#include "nvm/nvm.h"

#include "nvm_platform.h"
#include <stddef.h>
#include <string.h>

#define NVM_SECTOR_COUNT 2U
#define NVM_ENTRY_HEADER_SIZE 4U
#define NVM_MAX_TRACKED_IDS 64U
#define NVM_MAX_PAYLOAD_LEN NVM_MAGIC_LEN
#define NVM_MAX_ENTRY_LEN 16U
#define SECTOR_0 0x00U
#define SECTOR_1 0x01U
#define SECTOR_NONE 0xffU
#define OTHER_SECTOR(sector) (((sector) == SECTOR_0) ? SECTOR_1 : SECTOR_0)

typedef struct {
    uint16_t id;
    uint16_t offset;
} NVM_RecordRef;

typedef struct {
    uint32_t magic;
    uint16_t generation;
} NVM_MagicData;

typedef enum {
    NVM_SECTOR_INVALID,
    NVM_SECTOR_VALID,
    NVM_SECTOR_CORRUPT,
} NVM_SectorState;

typedef struct {
    uint8_t active_sector;
    uint16_t generation;
    NVM_RecordRef records[NVM_MAX_TRACKED_IDS];
    uint8_t record_count;
    bool record_overflow;
    uint16_t end_offset;
} nvm_t;

static nvm_t nvm = {0};

extern void Error_Handler(void);

static uint8_t type_size(const NVM_Type type) {
    switch (type) {
        case MAGIC:
            return NVM_MAGIC_LEN;
        case UINT8:
            return sizeof(uint8_t);
        case UINT16:
            return sizeof(uint16_t);
        default:
            return 0U;
    }
}

static uint16_t align_up(const uint16_t value, const uint8_t alignment) {
    return (uint16_t) (((uint32_t) value + alignment - 1U) / alignment * alignment);
}

static uint16_t entry_length(const NVM_Type type) {
    const uint8_t length = type_size(type);
    if (length == 0U) {
        return 0U;
    }

    return align_up(NVM_ENTRY_HEADER_SIZE + length, NVM_PlatformProgramUnitSize());
}

static uint32_t sector_address(const uint8_t sector) {
    return NVM_PlatformBase() + ((uint32_t) sector * NVM_PlatformSectorSize());
}

static bool generation_is_newer(const uint16_t candidate, const uint16_t current) {
    const uint16_t difference = candidate - current;
    return (difference != 0U) && (difference < 0x8000U);
}

static uint8_t select_newest_sector(const NVM_SectorState *states,
                                    const uint16_t *generations,
                                    const NVM_SectorState required_state) {
    if (states[SECTOR_0] != required_state) {
        return SECTOR_1;
    }
    if (states[SECTOR_1] != required_state) {
        return SECTOR_0;
    }

    if (generation_is_newer(generations[SECTOR_1], generations[SECTOR_0])) {
        return SECTOR_1;
    }

    return SECTOR_0;
}

static NVM_RecordRef *find_record(const uint16_t id) {
    for (uint8_t i = 0U; i < nvm.record_count; i++) {
        if (nvm.records[i].id == id) {
            return &nvm.records[i];
        }
    }

    return NULL;
}

static bool update_record(const uint16_t id, const uint16_t offset) {
    if (id == NVM_MAGIC_ID) {
        return true;
    }

    NVM_RecordRef *record = find_record(id);
    if (record != NULL) {
        record->offset = offset;
        return true;
    }

    if (nvm.record_count >= NVM_MAX_TRACKED_IDS) {
        nvm.record_overflow = true;
        return false;
    }

    nvm.records[nvm.record_count].id = id;
    nvm.records[nvm.record_count].offset = offset;
    nvm.record_count++;
    return true;
}

static bool program_entry(const uint32_t address, const uint8_t *encoded,
                          const uint16_t encoded_length) {
    const uint8_t unit = NVM_PlatformProgramUnitSize();

    if (unit == sizeof(uint16_t)) {
        /* Stage the identifier first so an interrupted H5 EDATA entry is detectable. */
        if (!NVM_PlatformProgram(address + 2U, encoded + 2U)) {
            return false;
        }
        for (uint16_t offset = NVM_ENTRY_HEADER_SIZE; offset < encoded_length; offset += unit) {
            if (!NVM_PlatformProgram(address + offset, encoded + offset)) {
                return false;
            }
        }
    } else {
        /* On G0, write any continuation lines before the atomic commit line. */
        for (uint16_t offset = unit; offset < encoded_length; offset += unit) {
            if (!NVM_PlatformProgram(address + offset, encoded + offset)) {
                return false;
            }
        }
    }

    /* The first program unit contains the header and commits the entry. */
    return NVM_PlatformProgram(address, encoded);
}

static bool write_entry(const uint8_t sector, const uint16_t offset, const NVM_Query *query) {
    const uint8_t length = type_size(query->type);
    const uint16_t encoded_length = entry_length(query->type);
    if ((length == 0U) || (encoded_length > NVM_MAX_ENTRY_LEN) ||
        (((uint32_t) offset + encoded_length) > NVM_PlatformSectorSize())) {
        return false;
    }

    uint8_t encoded[NVM_MAX_ENTRY_LEN] = {0};
    const uint16_t header = (uint16_t) query->type | ((uint16_t) length << 8U);
    encoded[0] = (uint8_t) header;
    encoded[1] = (uint8_t) (header >> 8U);
    encoded[2] = (uint8_t) query->id;
    encoded[3] = (uint8_t) (query->id >> 8U);
    memcpy(encoded + NVM_ENTRY_HEADER_SIZE, query->data, length);

    return program_entry(sector_address(sector) + offset, encoded, encoded_length);
}

static NVM_PlatformReadState read_entry(const uint8_t sector, const uint16_t offset,
                                        NVM_Query *entry) {
    const uint32_t address = sector_address(sector) + offset;
    uint16_t header;
    uint16_t id;

    NVM_PlatformReadState state = NVM_PlatformRead16(address, &header);
    if (state != NVM_PLATFORM_READ_OK) {
        return state;
    }
    state = NVM_PlatformRead16(address + 2U, &id);
    if (state != NVM_PLATFORM_READ_OK) {
        return NVM_PLATFORM_READ_CORRUPT;
    }

    const NVM_Type type = (NVM_Type) (header & UINT8_MAX);
    const uint8_t length = (uint8_t) (header >> 8U);
    const uint8_t expected_length = type_size(type);
    const uint16_t encoded_length = entry_length(type);
    if ((expected_length == 0U) || (length != expected_length) ||
        (((uint32_t) offset + encoded_length) > NVM_PlatformSectorSize())) {
        return NVM_PLATFORM_READ_CORRUPT;
    }

    uint8_t *data = entry->data;
    for (uint8_t i = 0U; i < length; i += 2U) {
        uint16_t value;
        if (NVM_PlatformRead16(address + NVM_ENTRY_HEADER_SIZE + i, &value) !=
            NVM_PLATFORM_READ_OK) {
            return NVM_PLATFORM_READ_CORRUPT;
        }

        data[i] = (uint8_t) value;
        if ((i + 1U) < length) {
            data[i + 1U] = (uint8_t) (value >> 8U);
        }
    }

    entry->type = type;
    entry->id = id;
    return NVM_PLATFORM_READ_OK;
}

static NVM_SectorState scan_sector(const uint8_t sector) {
    const uint32_t address = sector_address(sector);
    nvm.record_count = 0U;
    nvm.record_overflow = false;
    nvm.end_offset = 0U;

    NVM_MagicData magic = {0};
    NVM_Query magic_entry = {.data = &magic};
    if ((read_entry(sector, 0U, &magic_entry) != NVM_PLATFORM_READ_OK) ||
        (magic_entry.type != MAGIC) ||
        (magic_entry.id != NVM_MAGIC_ID) ||
        (magic.magic != NVM_MAGIC_DATA)) {
        return NVM_SECTOR_INVALID;
    }
    nvm.generation = magic.generation;

    uint16_t offset = entry_length(MAGIC);
    nvm.end_offset = offset;

    while (((uint32_t) offset + NVM_ENTRY_HEADER_SIZE) <= NVM_PlatformSectorSize()) {
        uint16_t header = UINT16_MAX;
        uint16_t id = UINT16_MAX;

        const NVM_PlatformReadState header_state = NVM_PlatformRead16(address + offset, &header);
        if (header_state == NVM_PLATFORM_READ_CORRUPT) {
            return NVM_SECTOR_CORRUPT;
        }
        if ((header_state == NVM_PLATFORM_READ_UNWRITTEN) || (header == UINT16_MAX)) {
            const NVM_PlatformReadState id_state =
                    NVM_PlatformRead16(address + offset + 2U, &id);
            if ((id_state == NVM_PLATFORM_READ_UNWRITTEN) ||
                ((id_state == NVM_PLATFORM_READ_OK) && (id == UINT16_MAX))) {
                return NVM_SECTOR_VALID;
            }
            return NVM_SECTOR_CORRUPT;
        }

        if (NVM_PlatformRead16(address + offset + 2U, &id) != NVM_PLATFORM_READ_OK) {
            return NVM_SECTOR_CORRUPT;
        }

        const NVM_Type type = (NVM_Type) (header & UINT8_MAX);
        const uint8_t length = (uint8_t) (header >> 8U);
        const uint8_t expected_length = type_size(type);
        const uint16_t encoded_length = entry_length(type);
        if ((expected_length == 0U) || (length != expected_length) ||
            (((uint32_t) offset + encoded_length) > NVM_PlatformSectorSize())) {
            return NVM_SECTOR_CORRUPT;
        }

        for (uint8_t i = 0U; i < length; i += 2U) {
            uint16_t value;
            if (NVM_PlatformRead16(address + offset + NVM_ENTRY_HEADER_SIZE + i, &value) !=
                NVM_PLATFORM_READ_OK) {
                return NVM_SECTOR_CORRUPT;
            }
        }

        if (!update_record(id, offset)) {
            return NVM_SECTOR_CORRUPT;
        }

        offset += encoded_length;
        nvm.end_offset = offset;
    }

    return NVM_SECTOR_VALID;
}

static bool compact_sector(const uint8_t src_sector, const uint8_t dst_sector) {
    uint16_t destination_generation = 0U;

    if (src_sector == SECTOR_NONE) {
        nvm.record_count = 0U;
        nvm.record_overflow = false;
        nvm.end_offset = 0U;
        nvm.generation = 0U;
    } else {
        const NVM_SectorState source_state = scan_sector(src_sector);
        if ((source_state == NVM_SECTOR_INVALID) || nvm.record_overflow) {
            return false;
        }
        destination_generation = nvm.generation + 1U;
    }

    if (!NVM_PlatformEraseSector(dst_sector)) {
        return false;
    }

    uint16_t dst_offset = entry_length(MAGIC);
    for (uint8_t record_index = 0U; record_index < nvm.record_count; record_index++) {
        const NVM_RecordRef record = nvm.records[record_index];
        uint8_t value[NVM_MAX_PAYLOAD_LEN] = {0};
        NVM_Query entry = {.data = value};
        if ((read_entry(src_sector, record.offset, &entry) != NVM_PLATFORM_READ_OK) ||
            (entry.id != record.id)) {
            return false;
        }

        if (!write_entry(dst_sector, dst_offset, &entry)) {
            return false;
        }
        dst_offset += entry_length(entry.type);
    }

    NVM_MagicData magic = {
        .magic = NVM_MAGIC_DATA,
        .generation = destination_generation,
    };
    const NVM_Query magic_query = {
        .type = MAGIC,
        .id = NVM_MAGIC_ID,
        .data = &magic,
    };
    if (!write_entry(dst_sector, 0U, &magic_query)) {
        return false;
    }

    if (scan_sector(dst_sector) != NVM_SECTOR_VALID) {
        return false;
    }

    nvm.active_sector = dst_sector;
    return true;
}

static bool init_journal(void) {
    NVM_SectorState states[NVM_SECTOR_COUNT] = {0};
    uint16_t generations[NVM_SECTOR_COUNT] = {0};

    for (uint8_t sector = 0U; sector < NVM_SECTOR_COUNT; sector++) {
        states[sector] = scan_sector(sector);
        if (states[sector] != NVM_SECTOR_INVALID) {
            generations[sector] = nvm.generation;
        }
    }

    if ((states[SECTOR_0] == NVM_SECTOR_VALID) ||
        (states[SECTOR_1] == NVM_SECTOR_VALID)) {
        nvm.active_sector = select_newest_sector(states, generations, NVM_SECTOR_VALID);
        return scan_sector(nvm.active_sector) == NVM_SECTOR_VALID;
    }

    if ((states[SECTOR_0] == NVM_SECTOR_CORRUPT) ||
        (states[SECTOR_1] == NVM_SECTOR_CORRUPT)) {
        const uint8_t source_sector = select_newest_sector(states, generations,
                                                           NVM_SECTOR_CORRUPT);
        return compact_sector(source_sector, OTHER_SECTOR(source_sector));
    }

    return compact_sector(SECTOR_NONE, SECTOR_0);
}

void NVM_Init(void) {
    if (!NVM_PlatformInit() || !init_journal()) {
        Error_Handler();
    }
}

bool NVM_Read(const NVM_Query *queries, const uint8_t query_count) {
    for (uint8_t query_index = 0U; query_index < query_count; query_index++) {
        const NVM_RecordRef *record = find_record(queries[query_index].id);
        if (record == NULL) {
            continue;
        }

        uint8_t value[NVM_MAX_PAYLOAD_LEN] = {0};
        NVM_Query entry = {.data = value};
        if (read_entry(nvm.active_sector, record->offset, &entry) != NVM_PLATFORM_READ_OK) {
            return false;
        }
        if ((entry.id != queries[query_index].id) ||
            (entry.type != queries[query_index].type)) {
            continue;
        }

        memcpy(queries[query_index].data, value, type_size(entry.type));
    }

    return true;
}

bool NVM_Write(const NVM_Query *query) {
    const uint16_t encoded_length = entry_length(query->type);
    if ((encoded_length == 0U) || (query->type == MAGIC) || (query->id == NVM_MAGIC_ID)) {
        return false;
    }

    if ((find_record(query->id) == NULL) && (nvm.record_count >= NVM_MAX_TRACKED_IDS)) {
        return false;
    }

    if (((uint32_t) nvm.end_offset + encoded_length) > NVM_PlatformSectorSize()) {
        const uint8_t destination_sector = OTHER_SECTOR(nvm.active_sector);
        if (!compact_sector(nvm.active_sector, destination_sector)) {
            return false;
        }
        if (((uint32_t) nvm.end_offset + encoded_length) > NVM_PlatformSectorSize()) {
            return false;
        }
    }

    uint16_t entry_offset = nvm.end_offset;
    if (!write_entry(nvm.active_sector, entry_offset, query)) {
        const uint8_t destination_sector = OTHER_SECTOR(nvm.active_sector);
        if (!compact_sector(nvm.active_sector, destination_sector)) {
            return false;
        }

        entry_offset = nvm.end_offset;
        if (!write_entry(nvm.active_sector, entry_offset, query)) {
            const uint8_t cleanup_sector = OTHER_SECTOR(nvm.active_sector);
            if (!compact_sector(nvm.active_sector, cleanup_sector)) {
                return false;
            }
            return false;
        }
    }

    if (!update_record(query->id, entry_offset)) {
        return false;
    }

    nvm.end_offset += encoded_length;
    return true;
}

bool NVM_HandleNMI(void) {
    return NVM_PlatformHandleNMI();
}
