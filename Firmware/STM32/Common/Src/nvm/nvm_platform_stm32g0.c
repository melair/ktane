#include "nvm_platform.h"

#include "stm32g0xx_hal.h"

#define NVM_FLASH_SECTOR_COUNT 2U
#define NVM_FLASH_SECTOR_SIZE FLASH_PAGE_SIZE
#define NVM_FLASH_FIRST_PAGE (FLASH_PAGE_NB - NVM_FLASH_SECTOR_COUNT)
#define NVM_FLASH_BASE_ADDRESS (FLASH_BASE + FLASH_SIZE - \
                                (NVM_FLASH_SECTOR_COUNT * NVM_FLASH_SECTOR_SIZE))

_Static_assert(FLASH_PAGE_SIZE == (2U * 1024U), "STM32G0 NVM requires 2 KiB pages");

extern uint8_t __nvm_start__;
extern uint8_t __nvm_end__;

static volatile bool read_active;
static volatile NVM_PlatformReadState read_state = NVM_PLATFORM_READ_OK;

bool NVM_PlatformInit(void) {
    return (FLASH_SIZE == (128U * 1024U)) &&
           (NVM_FLASH_BASE_ADDRESS == (FLASH_BASE + (124U * 1024U))) &&
           ((uint32_t) (uintptr_t) &__nvm_start__ == NVM_FLASH_BASE_ADDRESS) &&
           (((uint32_t) (uintptr_t) &__nvm_end__ -
             (uint32_t) (uintptr_t) &__nvm_start__) ==
            (NVM_FLASH_SECTOR_COUNT * NVM_FLASH_SECTOR_SIZE));
}

uint32_t NVM_PlatformBase(void) {
    return NVM_FLASH_BASE_ADDRESS;
}

uint16_t NVM_PlatformSectorSize(void) {
    return NVM_FLASH_SECTOR_SIZE;
}

uint8_t NVM_PlatformProgramUnitSize(void) {
    return sizeof(uint64_t);
}

NVM_PlatformReadState NVM_PlatformRead16(const uint32_t address, uint16_t *value) {
    read_state = NVM_PLATFORM_READ_OK;
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ECCC);
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ECCD);

    read_active = true;
    __DSB();
    *value = *(volatile const uint16_t *) address;
    __DSB();
    read_active = false;

    /* Corrected single-bit errors still return valid data. */
    if ((FLASH->ECCR & FLASH_ECCR_ECCC) != 0U) {
        __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ECCC);
    }
    return read_state;
}

bool NVM_PlatformProgram(const uint32_t address, const uint8_t *data) {
    uint64_t value = 0U;
    for (uint8_t i = 0U; i < sizeof(value); i++) {
        value |= (uint64_t) data[i] << (i * 8U);
    }

    if (HAL_FLASH_Unlock() != HAL_OK) {
        return false;
    }
    const HAL_StatusTypeDef program_status = HAL_FLASH_Program(
        FLASH_TYPEPROGRAM_DOUBLEWORD, address, value);
    const HAL_StatusTypeDef lock_status = HAL_FLASH_Lock();
    return (program_status == HAL_OK) && (lock_status == HAL_OK);
}

bool NVM_PlatformEraseSector(const uint8_t sector) {
    if (sector >= NVM_FLASH_SECTOR_COUNT) {
        return false;
    }
    if (HAL_FLASH_Unlock() != HAL_OK) {
        return false;
    }

    FLASH_EraseInitTypeDef erase = {
        .TypeErase = FLASH_TYPEERASE_PAGES,
        .Banks = FLASH_BANK_1,
        .Page = NVM_FLASH_FIRST_PAGE + sector,
        .NbPages = 1U,
    };
    uint32_t page_error = 0U;
    const HAL_StatusTypeDef erase_status = HAL_FLASHEx_Erase(&erase, &page_error);
    const HAL_StatusTypeDef lock_status = HAL_FLASH_Lock();
    return (erase_status == HAL_OK) && (page_error == UINT32_MAX) &&
           (lock_status == HAL_OK);
}

bool NVM_PlatformHandleNMI(void) {
    if (!read_active || ((FLASH->ECCR & FLASH_ECCR_ECCD) == 0U)) {
        return false;
    }

    read_state = NVM_PLATFORM_READ_CORRUPT;
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ECCD);
    return true;
}
