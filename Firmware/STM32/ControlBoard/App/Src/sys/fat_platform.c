#include "fat/fat_platform.h"
#include "sys/sdcard.h"

static uint32_t sector_count(void) {
    const SD_HandleTypeDef *handle = SDCard_GetHandle();
    return handle != NULL ? handle->SdCard.LogBlockNbr : 0;
}

const FAT_Platform FAT_PLATFORM = {
    .queue = SDCard_Queue,
    .sector_count = sector_count,
    .buffer_valid = SDCard_BufferValid,
    .get_tick = HAL_GetTick,
};
