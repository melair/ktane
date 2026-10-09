#ifndef SDCARD_H
#define SDCARD_H

#include <stdbool.h>
#include "stm32h5xx_hal.h"

typedef enum {
    SDCARD_STATE_OFF,
    SDCARD_STATE_POWERING,
    SDCARD_STATE_INITIALISING,
    SDCARD_STATE_READY,
    SDCARD_STATE_ERROR,
} SDCard_State;

typedef enum {
    SDCARD_STATUS_SUCCESS,
    SDCARD_STATUS_UNAVAILABLE,
    SDCARD_STATUS_CANCELLED,
    SDCARD_STATUS_TIMEOUT,
    SDCARD_STATUS_TRANSFER_ERROR,
    SDCARD_STATUS_PENDING,
    SDCARD_STATUS_INVALID,
} SDCard_Status;

typedef enum {
    SDCARD_OPERATION_READ,
    SDCARD_OPERATION_WRITE,
} SDCard_Operation;

typedef struct SDCard_Transaction SDCard_Transaction;

struct SDCard_Transaction {
    SDCard_Operation operation;
    volatile SDCard_Status status;
    uint32_t sector;
    uint32_t sector_count;
    void *data;
    SDCard_Transaction *(*callback)(SDCard_Transaction *);
    void *callback_data;
    SDCard_Transaction *queue_next;
};

typedef struct {
    uint32_t bus_clock_hz; /* Actual configured SDMMC bus clock after init. */
    uint32_t bus_width_bits; /* Negotiated data width: 1 or 4. */
    uint32_t card_type;    /* HAL CARD_SDSC or CARD_SDHC_SDXC. */
    uint32_t card_speed;   /* HAL CARD_*_SPEED category, not the operating clock. */
    bool high_speed;       /* CMD6 confirmed high-speed timing (up to 50 MHz). */
} SDCard_Info;

/* Call after GPIO_Init() and PeripheralClock_Init(), then service in the main loop.
 * Power-up and card negotiation are asynchronous, capped at 5s from power-on.
 * All APIs are main-loop only. Failed cards are retried after removal/reinsertion.
 */
void SDCard_Init(void);
void SDCard_Service(void);
bool SDCard_IsPresent(void);
SDCard_State SDCard_GetState(void);
uint32_t SDCard_GetError(void);
/* Returns NULL unless ready and present; valid until the next service call.
 * HAL groups SDHC and SDXC together under CARD_SDHC_SDXC.
 */
const SDCard_Info *SDCard_GetInfo(void);

/* Queue a read or write of sector_count consecutive 512-byte sectors at sector
 * (zero-based). sector_count must be nonzero. data must be 4-byte aligned SRAM
 * and remain alive and untouched until completion. The caller guarantees
 * room for sector_count * 512 bytes; the driver cannot check allocation size.
 * Write buffers are never modified by the driver. A failed read may be partial.
 * false rejects invalid/unavailable transactions without modifying them or
 * invoking a callback. true sets PENDING; completion status and the optional
 * callback are delivered by a later SDCard_Service(). There is no queue limit.
 * Keep the transaction alive and unmodified while pending; never submit it
 * twice or queue an object also returned by a callback.
 * A callback returns a follow-on transaction (possibly the same object), which
 * runs before queued work. NULL resumes FIFO processing. Invalid follow-ons
 * complete with INVALID on a subsequent service pass. NULL callbacks allow
 * status polling. Card loss clears outstanding work and ignores follow-ons
 * returned by failure callbacks. Do not recursively call Service().
 */
bool SDCard_Queue(SDCard_Transaction *transaction);

/* Handle is available only while ready and present. Do not retain it across
 * service calls or access the peripheral while sector requests are outstanding.
 */
SD_HandleTypeDef *SDCard_GetHandle(void);
void SDCard_Activity(void);
void SDCard_ReportError(uint32_t error);

#endif // SDCARD_H
