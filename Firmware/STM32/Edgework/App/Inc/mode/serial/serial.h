#ifndef EDGEWORK_MODE_SERIAL_H
#define EDGEWORK_MODE_SERIAL_H

#include <stdint.h>

typedef struct Mode_Definition Mode_Definition;

#define SERIAL_EPAPER_WIDTH  298U
#define SERIAL_EPAPER_HEIGHT 128U
#define SERIAL_EPAPER_FRAMEBUFFER_SIZE \
    ((SERIAL_EPAPER_WIDTH * SERIAL_EPAPER_HEIGHT) / 8U)

typedef struct {
    struct {
        uint8_t black[SERIAL_EPAPER_FRAMEBUFFER_SIZE];
        uint8_t red[SERIAL_EPAPER_FRAMEBUFFER_SIZE];
    } epaper;
} Serial_Data;

extern Mode_Definition serial_mode;

#endif // EDGEWORK_MODE_SERIAL_H
