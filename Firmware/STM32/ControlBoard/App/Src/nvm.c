#include "nvm.h"

#include "stm32h5xx_hal.h"
#include "stm32h5xx_it.h"
#include <stdbool.h>

#define NVM_EDATA_SECTOR_COUNT 2
#define NVM_EDATA_SECTOR_SIZE (6 * 1024)
#define NVM_EDATA_BASE (FLASH_EDATA_BASE + FLASH_EDATA_BANK_SIZE - \
                        (NVM_EDATA_SECTOR_COUNT * NVM_EDATA_SECTOR_SIZE))
#define NVM_ENTRY_HEADER_SIZE 4
#define NVM_ENTRY_PADDING_VALUE 0x00
#define NVM_MAX_TRACKED_IDS 64
#define NVM_MAX_PAYLOAD_LEN 6
#define SECTOR_0 0x00
#define SECTOR_1 0x01
#define SECTOR_NONE 0xff
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
    NVM_READ_OK,
    NVM_READ_UNWRITTEN,
    NVM_READ_CORRUPT,
} NVM_ReadState;

typedef enum {
    NVM_SECTOR_INVALID,
    NVM_SECTOR_VALID,
    NVM_SECTOR_CORRUPT,
} NVM_SectorState;

typedef struct {
    volatile bool read_active;
    volatile NVM_ReadState read_state;
    uint8_t active_sector;
    uint16_t generation;
    NVM_RecordRef records[NVM_MAX_TRACKED_IDS];
    uint8_t record_count;
    bool record_overflow;
    uint16_t end_offset;
} nvm_t;

static nvm_t nvm = {
    .read_state = NVM_READ_OK,
};

static NVM_ReadState read16(const uint32_t address, uint16_t *value) {
    nvm.read_state = NVM_READ_OK;
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ECCR_ERRORS);

    nvm.read_active = true;
    __DSB();
    *value = *(volatile const uint16_t *) address;
    __DSB();
    nvm.read_active = false;

    return nvm.read_state;
}

static bool write16(uint32_t address, uint16_t value) {
    uint32_t data = value;

    if (HAL_FLASH_Unlock() != HAL_OK) {
        return false;
    }

    const HAL_StatusTypeDef program_status = HAL_FLASH_Program(
        FLASH_TYPEPROGRAM_HALFWORD_EDATA,
        address,
        (uintptr_t) &data
    );
    const HAL_StatusTypeDef lock_status = HAL_FLASH_Lock();

    return (program_status == HAL_OK) && (lock_status == HAL_OK);
}

static uint8_t type_size(const NVM_Type type) {
    switch (type) {
        case MAGIC:
            return NVM_MAGIC_LEN;
        case UINT8:
            return sizeof(uint8_t);
        case UINT16:
            return sizeof(uint16_t);
        default:
            return 0;
    }
}

static bool generation_is_newer(const uint16_t candidate, const uint16_t current) {
    const uint16_t difference = candidate - current;
    return (difference != 0) && (difference < 0x8000);
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
    for (uint8_t i = 0; i < nvm.record_count; i++) {
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

static bool write_entry(const uint8_t sector, const uint16_t offset, const NVM_Query *query) {
    const uint8_t length = type_size(query->type);
    if (length == 0) {
        return false;
    }

    const uint16_t padded_length = (uint16_t) ((length + 1) & ~1);
    if (((uint32_t) offset + NVM_ENTRY_HEADER_SIZE + padded_length) > NVM_EDATA_SECTOR_SIZE) {
        return false;
    }

    const uint32_t address = NVM_EDATA_BASE + ((uint32_t) sector * NVM_EDATA_SECTOR_SIZE) + offset;
    if (!write16(address + 2, query->id)) {
        return false;
    }

    const uint8_t *payload = query->data;
    for (uint8_t i = 0; i < length; i += 2) {
        uint16_t value = payload[i];
        if ((i + 1) < length) {
            value |= (uint16_t) payload[i + 1] << 8;
        } else {
            value |= (uint16_t) NVM_ENTRY_PADDING_VALUE << 8;
        }

        if (!write16(address + NVM_ENTRY_HEADER_SIZE + i, value)) {
            return false;
        }
    }

    /* Commit the entry only after its identifier and payload have been written. */
    return write16(address, (uint16_t) query->type | ((uint16_t) length << 8));
}

static NVM_ReadState read_entry(const uint8_t sector, const uint16_t offset, NVM_Query *entry) {
    const uint32_t address = NVM_EDATA_BASE + ((uint32_t) sector * NVM_EDATA_SECTOR_SIZE) + offset;
    uint16_t header;
    uint16_t id;

    const NVM_ReadState header_state = read16(address, &header);
    if (header_state != NVM_READ_OK) {
        return header_state;
    }

    if (read16(address + 2, &id) != NVM_READ_OK) {
        return NVM_READ_CORRUPT;
    }

    const NVM_Type type = (NVM_Type) (header & UINT8_MAX);
    const uint8_t length = (uint8_t) (header >> 8);
    const uint8_t expected_length = type_size(type);
    if ((expected_length == 0) || (length != expected_length)) {
        return NVM_READ_CORRUPT;
    }

    const uint16_t padded_length = (uint16_t) ((length + 1) & ~1);
    if (((uint32_t) offset + NVM_ENTRY_HEADER_SIZE + padded_length) > NVM_EDATA_SECTOR_SIZE) {
        return NVM_READ_CORRUPT;
    }

    uint8_t *data = entry->data;
    for (uint8_t i = 0; i < length; i += 2) {
        uint16_t value;
        if (read16(address + NVM_ENTRY_HEADER_SIZE + i, &value) != NVM_READ_OK) {
            return NVM_READ_CORRUPT;
        }

        data[i] = (uint8_t) value;
        if ((i + 1) < length) {
            data[i + 1] = (uint8_t) (value >> 8);
        }
    }

    entry->type = type;
    entry->id = id;
    return NVM_READ_OK;
}

static bool erase_sector(const uint32_t sector) {
    if (HAL_FLASH_Unlock() != HAL_OK) {
        return false;
    }

    FLASH_EraseInitTypeDef erase = {
        .TypeErase = FLASH_TYPEERASE_SECTORS,
        .Banks = FLASH_BANK_1,
        .Sector = sector,
        .NbSectors = 1,
    };
    uint32_t sector_error = 0;

    const HAL_StatusTypeDef erase_status = HAL_FLASHEx_Erase(&erase, &sector_error);
    const HAL_StatusTypeDef lock_status = HAL_FLASH_Lock();

    return (erase_status == HAL_OK) && (lock_status == HAL_OK);
}

static bool erase_journal_sector(const uint8_t sector) {
    const uint32_t first_edata_sector =
            (FLASH_BANK_SIZE / FLASH_SECTOR_SIZE) - NVM_EDATA_SECTOR_COUNT;
    return erase_sector(first_edata_sector + sector);
}

static bool handle_nvm_nmi(void) {
    const uint32_t expected_status = FLASH_ECCR_ECCD | FLASH_ECCR_DATA_ECC;
    const uint32_t status = FLASH->ECCDETR;

    if (!nvm.read_active || ((status & expected_status) != expected_status) ||
        ((status & FLASH_ECCR_BK_ECC) != 0)) {
        return false;
    }

    FLASH_EccInfoTypeDef error = {0};
    HAL_FLASHEx_GetEccInfo(&error);
    if (error.Area != FLASH_ECC_AREA_EDATA_BANK1) {
        return false;
    }

    if (error.Data == FLASH_ECCDR_FAIL_DATA) {
        nvm.read_state = NVM_READ_UNWRITTEN;
    } else {
        nvm.read_state = NVM_READ_CORRUPT;
    }

    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ECCD);
    return true;
}

void NMI_Handler(void) {
    if (!handle_nvm_nmi()) {
        Error_Handler();
    }
}

static void init_flash(void) {
    FLASH_OBProgramInitTypeDef option_bytes = {
        .Banks = FLASH_BANK_1,
    };

    HAL_FLASHEx_OBGetConfig(&option_bytes);
    if (option_bytes.EDATASize == NVM_EDATA_SECTOR_COUNT) {
        return;
    }

    if (HAL_FLASH_OB_Unlock() != HAL_OK) {
        Error_Handler();
    }

    option_bytes.OptionType = OPTIONBYTE_EDATA;
    option_bytes.Banks = FLASH_BANK_1;
    option_bytes.EDATASize = NVM_EDATA_SECTOR_COUNT;
    if (HAL_FLASHEx_OBProgram(&option_bytes) != HAL_OK) {
        Error_Handler();
    }

    /* Reloading option bytes normally resets the MCU. */
    if (HAL_FLASH_OB_Launch() != HAL_OK) {
        Error_Handler();
    }

    HAL_FLASH_OB_Lock();

    const uint32_t first_edata_sector =
            (FLASH_BANK_SIZE / FLASH_SECTOR_SIZE) - NVM_EDATA_SECTOR_COUNT;
    for (uint32_t offset = 0; offset < NVM_EDATA_SECTOR_COUNT; offset++) {
        if (!erase_sector(first_edata_sector + offset)) {
            Error_Handler();
        }
    }

    /* Recommended to restart the system after a change of option bytes. */
    NVIC_SystemReset();
}

static NVM_SectorState scan_sector(const uint8_t sector) {
    const uint32_t sector_address = NVM_EDATA_BASE + ((uint32_t) sector * NVM_EDATA_SECTOR_SIZE);
    nvm.record_count = 0;
    nvm.record_overflow = false;
    nvm.end_offset = 0;

    NVM_MagicData magic = {0};
    NVM_Query magic_entry = {
        .data = &magic,
    };
    if ((read_entry(sector, 0, &magic_entry) != NVM_READ_OK) ||
        (magic_entry.type != MAGIC) ||
        (magic_entry.id != NVM_MAGIC_ID) ||
        (magic.magic != NVM_MAGIC_DATA)) {
        return NVM_SECTOR_INVALID;
    }
    nvm.generation = magic.generation;

    const uint8_t magic_length = type_size(MAGIC);
    uint16_t offset = NVM_ENTRY_HEADER_SIZE + (uint16_t) ((magic_length + 1) & ~1);
    nvm.end_offset = offset;

    while (((uint32_t) offset + NVM_ENTRY_HEADER_SIZE) <= NVM_EDATA_SECTOR_SIZE) {
        uint16_t header;
        uint16_t id;

        /* An unwritten header is only a clean end if the staged identifier is also unwritten. */
        const NVM_ReadState header_state = read16(sector_address + offset, &header);
        if (header_state == NVM_READ_UNWRITTEN) {
            const NVM_ReadState id_state = read16(sector_address + offset + 2, &id);
            if (id_state == NVM_READ_UNWRITTEN) {
                return NVM_SECTOR_VALID;
            }

            return NVM_SECTOR_CORRUPT;
        }
        if (header_state == NVM_READ_CORRUPT) {
            return NVM_SECTOR_CORRUPT;
        }

        if (read16(sector_address + offset + 2, &id) != NVM_READ_OK) {
            return NVM_SECTOR_CORRUPT;
        }

        const NVM_Type type = (NVM_Type) (header & UINT8_MAX);
        const uint8_t length = (uint8_t) (header >> 8);
        const uint8_t expected_length = type_size(type);
        if ((expected_length == 0) || (length != expected_length)) {
            return NVM_SECTOR_CORRUPT;
        }

        const uint16_t padded_length = (uint16_t) ((length + 1) & ~1);
        const uint16_t entry_length = NVM_ENTRY_HEADER_SIZE + padded_length;
        if (((uint32_t) offset + entry_length) > NVM_EDATA_SECTOR_SIZE) {
            return NVM_SECTOR_CORRUPT;
        }

        for (uint8_t i = 0; i < length; i += 2) {
            uint16_t value;
            if (read16(sector_address + offset + NVM_ENTRY_HEADER_SIZE + i, &value) != NVM_READ_OK) {
                return NVM_SECTOR_CORRUPT;
            }
        }

        if (!update_record(id, offset)) {
            return NVM_SECTOR_CORRUPT;
        }

        offset += entry_length;
        nvm.end_offset = offset;
    }

    return NVM_SECTOR_VALID;
}

/*
 * One size fits all, init the new sector, and read from the source, compacting src to dest.
 * if src is invalid, the dst sector is just inited. Use during init, and when sector would
 * breach size limits. */
static bool compact_sector(const uint8_t src_sector, const uint8_t dst_sector) {
    uint16_t destination_generation = 0;

    if (src_sector == SECTOR_NONE) {
        nvm.record_count = 0;
        nvm.record_overflow = false;
        nvm.end_offset = 0;
        nvm.generation = 0;
    } else {
        const NVM_SectorState source_state = scan_sector(src_sector);
        if ((source_state == NVM_SECTOR_INVALID) || nvm.record_overflow) {
            return false;
        }
        destination_generation = nvm.generation + 1;
    }

    if (!erase_journal_sector(dst_sector)) {
        return false;
    }

    const uint8_t magic_length = type_size(MAGIC);
    uint16_t dst_offset = NVM_ENTRY_HEADER_SIZE + (uint16_t) ((magic_length + 1) & ~1);

    for (uint8_t record_index = 0; record_index < nvm.record_count; record_index++) {
        const NVM_RecordRef record = nvm.records[record_index];
        uint8_t value[NVM_MAX_PAYLOAD_LEN] = {0};
        NVM_Query entry = {
            .data = value,
        };
        if ((read_entry(src_sector, record.offset, &entry) != NVM_READ_OK) ||
            (entry.id != record.id)) {
            return false;
        }

        if (!write_entry(dst_sector, dst_offset, &entry)) {
            return false;
        }

        const uint8_t length = type_size(entry.type);
        const uint16_t padded_length = (uint16_t) ((length + 1) & ~1);
        dst_offset += NVM_ENTRY_HEADER_SIZE + padded_length;
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
    if (!write_entry(dst_sector, 0, &magic_query)) {
        return false;
    }

    if (scan_sector(dst_sector) != NVM_SECTOR_VALID) {
        return false;
    }

    nvm.active_sector = dst_sector;
    return true;
}

static bool init_journal(void) {
    NVM_SectorState states[NVM_EDATA_SECTOR_COUNT] = {0};
    uint16_t generations[NVM_EDATA_SECTOR_COUNT] = {0};

    for (uint8_t sector = 0; sector < NVM_EDATA_SECTOR_COUNT; sector++) {
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
        const uint8_t source_sector =
                select_newest_sector(states, generations, NVM_SECTOR_CORRUPT);
        return compact_sector(source_sector, OTHER_SECTOR(source_sector));
    }

    return compact_sector(SECTOR_NONE, SECTOR_0);
}

void NVM_Init(void) {
    init_flash();
    if (!init_journal()) {
        Error_Handler();
    }
}

bool NVM_Read(const NVM_Query *queries, const uint8_t query_count) {
    for (uint8_t query_index = 0; query_index < query_count; query_index++) {
        const NVM_RecordRef *record = find_record(queries[query_index].id);
        if (record == NULL) {
            continue;
        }

        uint8_t value[NVM_MAX_PAYLOAD_LEN] = {0};
        NVM_Query entry = {
            .data = value,
        };
        if ((read_entry(nvm.active_sector, record->offset, &entry) != NVM_READ_OK) ||
            (entry.id != queries[query_index].id) ||
            (entry.type != queries[query_index].type)) {
            return false;
        }

        const uint8_t length = type_size(entry.type);
        uint8_t *destination = queries[query_index].data;
        for (uint8_t i = 0; i < length; i++) {
            destination[i] = value[i];
        }
    }

    return true;
}

bool NVM_Write(const NVM_Query *query) {
    const uint8_t length = type_size(query->type);
    if ((length == 0) || (query->type == MAGIC) || (query->id == NVM_MAGIC_ID)) {
        return false;
    }

    if ((find_record(query->id) == NULL) && (nvm.record_count >= NVM_MAX_TRACKED_IDS)) {
        return false;
    }

    const uint16_t padded_length = (uint16_t) ((length + 1) & ~1);
    const uint16_t entry_length = NVM_ENTRY_HEADER_SIZE + padded_length;

    if (((uint32_t) nvm.end_offset + entry_length) > NVM_EDATA_SECTOR_SIZE) {
        const uint8_t destination_sector = OTHER_SECTOR(nvm.active_sector);
        if (!compact_sector(nvm.active_sector, destination_sector)) {
            return false;
        }

        if (((uint32_t) nvm.end_offset + entry_length) > NVM_EDATA_SECTOR_SIZE) {
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

    nvm.end_offset += entry_length;
    return true;
}
