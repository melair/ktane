#include "nvm.h"

#include "stm32h5xx_hal.h"
#include "stm32h5xx_it.h"
#include <stdbool.h>

#define NVM_EDATA_SECTOR_COUNT 2
#define NVM_EDATA_SECTOR_SIZE (6 * 1024)
#define NVM_ENTRY_HEADER_SIZE 4
#define NVM_ENTRY_PADDING_VALUE 0x00
#define NVM_MAX_TRACKED_IDS 64
#define SECTOR_0 0x00
#define SECTOR_1 0x01
#define SECTOR_NONE 0xff

typedef struct {
    uint16_t id;
    uint16_t offset;
} NVM_RecordRef;

typedef struct {
    uint32_t magic;
    uint16_t generation;
} NVM_MagicData;

static volatile bool nvm_read_active = false;
static volatile bool nvm_read_failed = false;
static uint8_t nvm_active_sector = 0;
static uint16_t nvm_generation = 0;
static NVM_RecordRef nvm_records[NVM_MAX_TRACKED_IDS];
static uint8_t nvm_record_count = 0;
static bool nvm_record_overflow = false;
static uint16_t nvm_end_offset = 0;

static bool read16(uint32_t address, uint16_t *value) {
    nvm_read_failed = false;
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ECCR_ERRORS);

    nvm_read_active = true;
    __DSB();
    *value = *(volatile const uint16_t *) address;
    __DSB();
    nvm_read_active = false;

    return !nvm_read_failed;
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

static NVM_RecordRef *find_record(const uint16_t id) {
    for (uint8_t i = 0; i < nvm_record_count; i++) {
        if (nvm_records[i].id == id) {
            return &nvm_records[i];
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

    if (nvm_record_count >= NVM_MAX_TRACKED_IDS) {
        nvm_record_overflow = true;
        return false;
    }

    nvm_records[nvm_record_count].id = id;
    nvm_records[nvm_record_count].offset = offset;
    nvm_record_count++;
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

    const uint32_t address = FLASH_EDATA_BASE + ((uint32_t) sector * NVM_EDATA_SECTOR_SIZE) + offset;
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

static bool read_entry(const uint8_t sector, const uint16_t offset, NVM_Query *entry) {
    const uint32_t address = FLASH_EDATA_BASE + ((uint32_t) sector * NVM_EDATA_SECTOR_SIZE) + offset;
    uint16_t header;
    uint16_t id;

    if (!read16(address, &header) || !read16(address + 2, &id)) {
        return false;
    }

    const NVM_Type type = (NVM_Type) (header & UINT8_MAX);
    const uint8_t length = (uint8_t) (header >> 8);
    const uint8_t expected_length = type_size(type);
    if ((expected_length == 0) || (length != expected_length)) {
        return false;
    }

    const uint16_t padded_length = (uint16_t) ((length + 1) & ~1);
    if (((uint32_t) offset + NVM_ENTRY_HEADER_SIZE + padded_length) > NVM_EDATA_SECTOR_SIZE) {
        return false;
    }

    uint8_t *data = entry->data;
    for (uint8_t i = 0; i < length; i += 2) {
        uint16_t value;
        if (!read16(address + NVM_ENTRY_HEADER_SIZE + i, &value)) {
            return false;
        }

        data[i] = (uint8_t) value;
        if ((i + 1) < length) {
            data[i + 1] = (uint8_t) (value >> 8);
        }
    }

    entry->type = type;
    entry->id = id;
    return true;
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
    const uint32_t failed_data = FLASH->ECCDR & FLASH_ECCDR_FAIL_DATA;

    if (!nvm_read_active || ((status & expected_status) != expected_status) ||
        (failed_data != FLASH_ECCDR_FAIL_DATA)) {
        return false;
    }

    nvm_read_failed = true;
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

static bool scan_sector(const uint8_t sector) {
    const uint32_t sector_address = FLASH_EDATA_BASE + ((uint32_t) sector * NVM_EDATA_SECTOR_SIZE);
    nvm_record_count = 0;
    nvm_record_overflow = false;
    nvm_end_offset = 0;

    NVM_MagicData magic = {0};
    NVM_Query magic_entry = {
        .data = &magic,
    };
    if (!read_entry(sector, 0, &magic_entry) ||
        (magic_entry.type != MAGIC) ||
        (magic_entry.id != NVM_MAGIC_ID) ||
        (magic.magic != NVM_MAGIC_DATA)) {
        return false;
    }
    nvm_generation = magic.generation;

    const uint8_t magic_length = type_size(MAGIC);
    uint16_t offset = NVM_ENTRY_HEADER_SIZE + (uint16_t) ((magic_length + 1) & ~1);
    nvm_end_offset = offset;

    while (((uint32_t) offset + NVM_ENTRY_HEADER_SIZE) <= NVM_EDATA_SECTOR_SIZE) {
        uint16_t header;
        uint16_t id;

        /* If we fail to read the header, then it's likely not yet written, so end. */
        if (!read16(sector_address + offset, &header)) {
            return true;
        }

        if (!read16(sector_address + offset + 2, &id)) {
            return false;
        }

        const NVM_Type type = (NVM_Type) (header & UINT8_MAX);
        const uint8_t length = (uint8_t) (header >> 8);
        const uint8_t expected_length = type_size(type);
        if ((expected_length == 0) || (length != expected_length)) {
            return false;
        }

        const uint16_t padded_length = (uint16_t) ((length + 1) & ~1);
        const uint16_t entry_length = NVM_ENTRY_HEADER_SIZE + padded_length;
        if (((uint32_t) offset + entry_length) > NVM_EDATA_SECTOR_SIZE) {
            return false;
        }

        for (uint8_t i = 0; i < length; i += 2) {
            uint16_t value;
            if (!read16(sector_address + offset + NVM_ENTRY_HEADER_SIZE + i, &value)) {
                return false;
            }
        }

        if (!update_record(id, offset)) {
            return false;
        }

        offset += entry_length;
        nvm_end_offset = offset;
    }

    return true;
}

/*
 * One size fits all, init the new sector, and read from the source, compacting src to dest.
 * if src is invalid, the dst sector is just inited. Use during init, and when sector would
 * breach size limits. */
bool compact_sector(uint8_t src_sector, uint8_t dst_sector) {
    uint16_t destination_generation = 0;

    if (src_sector == SECTOR_NONE) {
        nvm_record_count = 0;
        nvm_record_overflow = false;
        nvm_end_offset = 0;
        nvm_generation = 0;
    } else {
        const bool source_valid = scan_sector(src_sector);
        if (!source_valid && nvm_record_overflow) {
            return false;
        }
        destination_generation = nvm_generation + 1;
    }

    if (!erase_journal_sector(dst_sector)) {
        return false;
    }

    const uint8_t magic_length = type_size(MAGIC);
    uint16_t dst_offset = NVM_ENTRY_HEADER_SIZE + (uint16_t) ((magic_length + 1) & ~1);

    for (uint8_t record_index = 0; record_index < nvm_record_count; record_index++) {
        const NVM_RecordRef record = nvm_records[record_index];
        uint8_t value[NVM_MAGIC_LEN] = {0};
        NVM_Query entry = {
            .data = value,
        };
        if (!read_entry(src_sector, record.offset, &entry) || (entry.id != record.id)) {
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

    if (!scan_sector(dst_sector)) {
        return false;
    }

    if ((src_sector != SECTOR_NONE) && !erase_journal_sector(src_sector)) {
        return false;
    }

    nvm_active_sector = dst_sector;
    return true;
}

void init_journal(void) {
    bool valid_sectors[NVM_EDATA_SECTOR_COUNT] = {0};
    uint16_t generations[NVM_EDATA_SECTOR_COUNT] = {0};

    for (uint8_t i = 0; i < NVM_EDATA_SECTOR_COUNT; i++) {
        valid_sectors[i] = scan_sector(i);
        if (valid_sectors[i]) {
            generations[i] = nvm_generation;
        }
    }

    if (valid_sectors[SECTOR_0] != valid_sectors[SECTOR_1]) {
        /* Only one sector is valid. */
        if (valid_sectors[SECTOR_0]) {
            nvm_active_sector = SECTOR_0;
        } else {
            nvm_active_sector = SECTOR_1;
        }

        scan_sector(nvm_active_sector);
    } else {
        if (valid_sectors[SECTOR_0]) {
            /* Both sectors are valid, retain the newest generation. */
            if (generation_is_newer(generations[SECTOR_1], generations[SECTOR_0])) {
                nvm_active_sector = SECTOR_1;
            } else {
                nvm_active_sector = SECTOR_0;
            }

            const uint8_t old_sector =
                    (nvm_active_sector == SECTOR_0) ? SECTOR_1 : SECTOR_0;
            erase_journal_sector(old_sector);
            scan_sector(nvm_active_sector);
        } else {
            /* Neither sectors are valid, wipe both. */
            erase_journal_sector(SECTOR_0);
            erase_journal_sector(SECTOR_1);
            compact_sector(SECTOR_NONE, SECTOR_0);
        }
    }
}

void NVM_Init(void) {
    init_flash();
    init_journal();

    if (!scan_sector(nvm_active_sector)) {
        Error_Handler();
    }
}

bool NVM_Read(const NVM_Query *queries, const uint8_t query_count) {
    for (uint8_t query_index = 0; query_index < query_count; query_index++) {
        const NVM_RecordRef *record = find_record(queries[query_index].id);
        if (record == NULL) {
            continue;
        }

        uint8_t value[NVM_MAGIC_LEN] = {0};
        NVM_Query entry = {
            .data = value,
        };
        if (!read_entry(nvm_active_sector, record->offset, &entry) ||
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

    if ((find_record(query->id) == NULL) && (nvm_record_count >= NVM_MAX_TRACKED_IDS)) {
        return false;
    }

    const uint16_t padded_length = (uint16_t) ((length + 1) & ~1);
    const uint16_t entry_length = NVM_ENTRY_HEADER_SIZE + padded_length;

    if (((uint32_t) nvm_end_offset + entry_length) > NVM_EDATA_SECTOR_SIZE) {
        const uint8_t destination_sector =
                (nvm_active_sector == SECTOR_0) ? SECTOR_1 : SECTOR_0;
        if (!compact_sector(nvm_active_sector, destination_sector)) {
            return false;
        }

        if (((uint32_t) nvm_end_offset + entry_length) > NVM_EDATA_SECTOR_SIZE) {
            return false;
        }
    }

    const uint16_t entry_offset = nvm_end_offset;
    if (!write_entry(nvm_active_sector, entry_offset, query)) {
        return false;
    }

    if (!update_record(query->id, entry_offset)) {
        return false;
    }

    nvm_end_offset += entry_length;
    return true;
}
