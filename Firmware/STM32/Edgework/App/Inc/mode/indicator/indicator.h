#ifndef EDGEWORK_MODE_INDICATOR_H
#define EDGEWORK_MODE_INDICATOR_H

#include <stdint.h>

typedef struct Mode_Definition Mode_Definition;

#define INDICATOR_EPAPER_WIDTH  250U
#define INDICATOR_EPAPER_HEIGHT 122U
#define INDICATOR_EPAPER_FRAMEBUFFER_SIZE \
    (((INDICATOR_EPAPER_WIDTH * INDICATOR_EPAPER_HEIGHT) + 7U) / 8U)

typedef struct {
    struct {
        uint8_t black[INDICATOR_EPAPER_FRAMEBUFFER_SIZE];
    } epaper;
} Indicator_Data;

extern Mode_Definition indicator_mode;

#endif // EDGEWORK_MODE_INDICATOR_H
