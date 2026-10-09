#ifndef SDCARD_INIT_H
#define SDCARD_INIT_H

#include "sys/sdcard.h"

/* Shared power-on deadline enforced by the lifecycle and negotiation FSMs. */
#define SDCARD_INIT_TIMEOUT_MS 5000U

typedef enum {
    SDCARD_INIT_PENDING,
    SDCARD_INIT_READY,
    SDCARD_INIT_FAILED,
} SDCard_InitResult;

/* Internal, main-loop-only interface for the single SDMMC1 card.
 * The caller retains handle/info ownership for the entire negotiation.
 * On failure, handle->ErrorCode contains the reason.
 */
void SDCard_InitStart(SD_HandleTypeDef *handle, SDCard_Info *info, uint32_t power_on_at);
SDCard_InitResult SDCard_InitService(void);
/* Stop negotiation and release the host, including after successful init. */
void SDCard_InitStop(SD_HandleTypeDef *handle);
/* Disconnect host signals and pulls before switching card power off. */
void SDCard_InitDisconnectBus(void);
/* Decode an R1 response without waiting for hardware. */
uint32_t SDCard_ResponseError(uint32_t response);

#endif // SDCARD_INIT_H
