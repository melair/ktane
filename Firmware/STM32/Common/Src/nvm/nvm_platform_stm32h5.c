#include "nvm_platform.h"

#include "stm32h5xx_hal.h"
#include <stddef.h>

#define NVM_EDATA_SECTOR_COUNT 2U
#define NVM_EDATA_SECTOR_SIZE (6U * 1024U)
#define NVM_EDATA_FIRST_FLASH_SECTOR (FLASH_SECTOR_NB - NVM_EDATA_SECTOR_COUNT)
#define NVM_EDATA_BASE_ADDRESS (FLASH_EDATA_BASE + FLASH_EDATA_BANK_SIZE - \
                                (NVM_EDATA_SECTOR_COUNT * NVM_EDATA_SECTOR_SIZE))

static volatile bool read_active;
static volatile NVM_PlatformReadState read_state = NVM_PLATFORM_READ_OK;

static bool erase_flash_sector(const uint32_t sector) {
    if (HAL_FLASH_Unlock() != HAL_OK) {
        return false;
    }

    FLASH_EraseInitTypeDef erase = {
        .TypeErase = FLASH_TYPEERASE_SECTORS,
        .Banks = FLASH_BANK_1,
        .Sector = sector,
        .NbSectors = 1U,
    };
    uint32_t sector_error = 0U;
    const HAL_StatusTypeDef erase_status = HAL_FLASHEx_Erase(&erase, &sector_error);
    const HAL_StatusTypeDef lock_status = HAL_FLASH_Lock();
    return (erase_status == HAL_OK) && (lock_status == HAL_OK);
}

bool NVM_PlatformInit(void) {
    FLASH_OBProgramInitTypeDef option_bytes = {.Banks = FLASH_BANK_1};
    HAL_FLASHEx_OBGetConfig(&option_bytes);
    if (option_bytes.EDATASize == NVM_EDATA_SECTOR_COUNT) {
        return true;
    }

    if (HAL_FLASH_OB_Unlock() != HAL_OK) {
        return false;
    }

    option_bytes.OptionType = OPTIONBYTE_EDATA;
    option_bytes.Banks = FLASH_BANK_1;
    option_bytes.EDATASize = NVM_EDATA_SECTOR_COUNT;
    if (HAL_FLASHEx_OBProgram(&option_bytes) != HAL_OK) {
        HAL_FLASH_OB_Lock();
        return false;
    }

    /* Launch normally resets the MCU. */
    if (HAL_FLASH_OB_Launch() != HAL_OK) {
        HAL_FLASH_OB_Lock();
        return false;
    }
    HAL_FLASH_OB_Lock();

    for (uint32_t sector = 0U; sector < NVM_EDATA_SECTOR_COUNT; sector++) {
        if (!erase_flash_sector(NVM_EDATA_FIRST_FLASH_SECTOR + sector)) {
            return false;
        }
    }

    NVIC_SystemReset();
    return false;
}

uint32_t NVM_PlatformBase(void) {
    return NVM_EDATA_BASE_ADDRESS;
}

uint16_t NVM_PlatformSectorSize(void) {
    return NVM_EDATA_SECTOR_SIZE;
}

uint8_t NVM_PlatformProgramUnitSize(void) {
    return sizeof(uint16_t);
}

NVM_PlatformReadState NVM_PlatformRead16(const uint32_t address, uint16_t *value) {
    read_state = NVM_PLATFORM_READ_OK;
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ECCR_ERRORS);

    read_active = true;
    __DSB();
    *value = *(volatile const uint16_t *) address;
    __DSB();
    read_active = false;
    return read_state;
}

bool NVM_PlatformProgram(const uint32_t address, const uint8_t *data) {
    const uint16_t value = (uint16_t) data[0] | ((uint16_t) data[1] << 8U);
    uint32_t program_data = value;

    if (HAL_FLASH_Unlock() != HAL_OK) {
        return false;
    }
    const HAL_StatusTypeDef program_status = HAL_FLASH_Program(
        FLASH_TYPEPROGRAM_HALFWORD_EDATA, address, (uintptr_t) &program_data);
    const HAL_StatusTypeDef lock_status = HAL_FLASH_Lock();
    return (program_status == HAL_OK) && (lock_status == HAL_OK);
}

bool NVM_PlatformEraseSector(const uint8_t sector) {
    return erase_flash_sector(NVM_EDATA_FIRST_FLASH_SECTOR + sector);
}

bool NVM_PlatformHandleNMI(void) {
    const uint32_t expected_status = FLASH_ECCR_ECCD | FLASH_ECCR_DATA_ECC;
    const uint32_t status = FLASH->ECCDETR;
    if (!read_active || ((status & expected_status) != expected_status) ||
        ((status & FLASH_ECCR_BK_ECC) != 0U)) {
        return false;
    }

    FLASH_EccInfoTypeDef error = {0};
    HAL_FLASHEx_GetEccInfo(&error);
    if (error.Area != FLASH_ECC_AREA_EDATA_BANK1) {
        return false;
    }

    read_state = (error.Data == FLASH_ECCDR_FAIL_DATA)
                     ? NVM_PLATFORM_READ_UNWRITTEN
                     : NVM_PLATFORM_READ_CORRUPT;
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ECCD);
    return true;
}
