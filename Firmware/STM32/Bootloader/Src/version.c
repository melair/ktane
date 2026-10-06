#include "board_identity.h"

const version_t firmware_version
    __attribute__((section(".firmware_version"), used, aligned(1))) = {
        .magic = VERSION_MAGIC,
        .crc32 = VERSION_UNCALCULATED_CRC32,
        .platform = BOARD_PLATFORM,
        .hardware = BOARD_HARDWARE,
        .type = VERSION_TYPE_BOOTLOADER,
        .debug_build = VERSION_DEBUG_BUILD,
        .version = 1,
        .image_length = (uint32_t)(uintptr_t)__application_image_length,
    };
