#ifndef SEGMENTS_H
#define	SEGMENTS_H

#define SEGMENT_COUNT 3

#define BOOTLOADER  0
#define APPLICATION 1
#define FLASHER     2

#define BOOTLOADER_OFFSET   0x00000
#define BOOTLOADER_SIZE     0x00100

#define APPLICATION_OFFSET  0x00100
#define APPLICATION_SIZE    0x1ef00

#define FLASHER_OFFSET      0x1f000
#define FLASHER_SIZE        0x01000

#endif	/* SEGMENTS_H */

