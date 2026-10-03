#include "version/version.h"

/* This firmware owns its descriptor. used preserves compiler output; KEEP
 * preserves it during linker collection. The linker reserves 128 bytes and
 * fills the bytes after this packed descriptor with 0xFF.
 */
const version_t firmware_version
    __attribute__((section(".firmware_version"), used, aligned(1))) = {
        .magic = VERSION_MAGIC,
        .crc32 = VERSION_UNCALCULATED_CRC32,
        .platform = VERSION_PLATFORM_EDGEWORK,
        .hardware = VERSION_HARDWARE('B', 1),
        .type = VERSION_TYPE_APPLICATION,
        .debug_build = VERSION_DEBUG_BUILD,
        .version = 0,
        .image_length = (uint32_t)(uintptr_t)__application_image_length,
    };
