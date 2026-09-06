#ifndef NVM_PLATFORM_H
#define NVM_PLATFORM_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    NVM_PLATFORM_READ_OK,
    NVM_PLATFORM_READ_UNWRITTEN,
    NVM_PLATFORM_READ_CORRUPT,
} NVM_PlatformReadState;

bool NVM_PlatformInit(void);
uint32_t NVM_PlatformBase(void);
uint16_t NVM_PlatformSectorSize(void);
uint8_t NVM_PlatformProgramUnitSize(void);
NVM_PlatformReadState NVM_PlatformRead16(uint32_t address, uint16_t *value);
bool NVM_PlatformProgram(uint32_t address, const uint8_t *data);
bool NVM_PlatformEraseSector(uint8_t sector);
bool NVM_PlatformHandleNMI(void);

#endif // NVM_PLATFORM_H
