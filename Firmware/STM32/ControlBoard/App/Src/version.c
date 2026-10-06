#include "version/version.h"

const version_t firmware_version
    __attribute__((section(".firmware_version"), used, aligned(1))) = {
        .magic = VERSION_MAGIC,
        .crc32 = VERSION_UNCALCULATED_CRC32,
        .platform = VERSION_PLATFORM_CONTROLBOARD,
        .hardware = VERSION_HARDWARE('C', 1),
        .type = VERSION_TYPE_APPLICATION,
        .debug_build = VERSION_DEBUG_BUILD,
        .version = 1,
        .image_length = (uint32_t)(uintptr_t)__application_image_length,
    };
