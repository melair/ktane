#ifndef VERSION_VERSION_H
#define VERSION_VERSION_H

#include <stddef.h>
#include <stdint.h>

#define VERSION_MAGIC UINT32_C(0x424D4F42)
#define VERSION_UNCALCULATED_CRC32 UINT32_C(0xFFFFFFFF)

typedef enum {
    VERSION_PLATFORM_UNSPECIFIED = 0,
    VERSION_PLATFORM_EDGEWORK = 1,
    VERSION_PLATFORM_BACKPLANE = 2,
    VERSION_PLATFORM_CONTROLBOARD = 3,
    VERSION_PLATFORM_COMMS = 4,
} version_platform_t;

typedef enum {
    VERSION_TYPE_BOOTLOADER = 0,
    VERSION_TYPE_APPLICATION = 1,
} version_application_t;

/* Hardware: A=1 through O=15 in the high nibble, revision 0..15 in the
 * low nibble. Display the numeric revision with two decimal digits:
 * VERSION_HARDWARE('C', 1) encodes C01 as 0x31. Zero means unspecified.
 */
#define VERSION_HARDWARE_UNSPECIFIED 0u
#define VERSION_HARDWARE(letter, number) \
    ((uint8_t)((((letter) - 'A' + 1u) << 4u) | (number)))

/* CMake defines DEBUG for Debug builds on every firmware target. */
#if defined(DEBUG)
#define VERSION_DEBUG_BUILD 1u
#else
#define VERSION_DEBUG_BUILD 0u
#endif

/* Enum values are stored as bytes to keep the on-flash layout independent of
 * enum width. Packed multi-byte fields should be accessed through this type,
 * rather than taking their addresses as naturally aligned integer pointers.
 * The linker independently reserves 128 bytes and fills the unused tail FF.
 * STM32 stores all multi-byte fields in little-endian byte order.
 */
typedef struct __attribute__((packed)) {
    uint32_t magic;           /* VERSION_MAGIC */
    uint32_t crc32;           /* VERSION_UNCALCULATED_CRC32 until calculated. */
    uint8_t platform;         /* version_platform_t */
    uint8_t hardware;         /* Letter in high nibble; number in low nibble. */
    uint8_t type;             /* version_application_t */
    uint8_t debug_build;      /* 0 = Release, 1 = Debug */
    uint16_t version;
    uint32_t image_length;        /* Flash image bytes, including header, gaps
                                  * and initialised RAM data; excludes NVM. */
} version_t;

_Static_assert(sizeof(version_t) == 18, "Firmware descriptor layout must be packed");
_Static_assert(offsetof(version_t, magic) == 0, "Firmware magic must be first");
_Static_assert(offsetof(version_t, crc32) == 4, "Firmware CRC must follow magic");
_Static_assert(offsetof(version_t, version) == 12, "Firmware version offset changed");
_Static_assert(offsetof(version_t, image_length) == 14, "Image length field moved");

/* Absolute linker symbol: its address encodes a value, not storage to read.
 * This populates the descriptor at link time, without runtime initialisation.
 */
extern const uint8_t __application_image_length[];

extern const version_t firmware_version;

#endif /* VERSION_VERSION_H */
