#include "sys/sdcard.h"
#include "sys/sdcard_init.h"

#include <string.h>
#include "fsm/fsm.h"
#include "sys/gpio.h"

#define SDCARD_DEBOUNCE_MS 500U
/* SD Physical Layer spec 6.4.1: up to 35ms ramp + >=1ms stable.
 * Use 40ms to allow tick quantisation; startup clocks are waited separately.
 * https://www.sdcard.org/downloads/pls/
 */
#define SDCARD_POWER_SETTLE_MS 40U
/* Board supply must actually discharge below 0.5V for >=1ms when off. */
#define SDCARD_POWER_OFF_MS 40U
#define SDCARD_ERROR_HALF_PERIOD_MS 250U
#define SDCARD_ACTIVITY_MS 25U
#define SDCARD_INIT_HALF_PERIOD_MS 125U
#define SDCARD_SECTOR_SIZE 512U
#define SDCARD_TRANSFER_TIMEOUT_MS 1000U

typedef enum { IO_IDLE, IO_COMMAND, IO_DATA, IO_STOP, IO_READY_WAIT, IO_STATUS } SDCard_IOState;

static struct {
    FSM fsm;
    uint32_t init_at;
    SD_HandleTypeDef handle;
    SDCard_Info info;
    uint32_t error;
    uint32_t off_at;
    uint32_t present_at;
    uint32_t error_at;
    uint32_t activity_at;
    uint32_t activity_started_at;
    bool detecting;
    bool boot_card_present;
    bool activity;
    SDCard_Transaction *queue_head;
    SDCard_Transaction *queue_tail;
    SDCard_Transaction *active_transaction;
    uint32_t io_at;
    uint32_t retry_at;
    SDCard_IOState io_state;
    SDCard_Status failure_status;
} sdcard;

static void led_set(bool on) {
    HAL_GPIO_WritePin(SDMMC_ACT_Port, SDMMC_ACT_Pin, on ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

static void power_off(void) {
    SDCard_InitDisconnectBus();
    HAL_GPIO_WritePin(SDMMC_PWR_Port, SDMMC_PWR_Pin, GPIO_PIN_RESET);
    sdcard.off_at = HAL_GetTick();
    sdcard.activity = false;
    memset(&sdcard.info, 0, sizeof(sdcard.info));
}

bool SDCard_IsPresent(void) {
    return HAL_GPIO_ReadPin(SDMMC_DET_Port, SDMMC_DET_Pin) == GPIO_PIN_RESET;
}

static void off_enter(FSM *fsm) {
    (void)fsm;
    power_off();
    led_set(false);
    sdcard.detecting = false;
}

static void off_service(FSM *fsm) {
    if (!SDCard_IsPresent()) {
        sdcard.detecting = false;
        sdcard.boot_card_present = false;
        return;
    }
    const uint32_t now = HAL_GetTick();
    if (!sdcard.detecting) {
        sdcard.detecting = true;
        sdcard.present_at = now;
    }
    if ((sdcard.boot_card_present || now - sdcard.present_at >= SDCARD_DEBOUNCE_MS) &&
        (now - sdcard.off_at >= SDCARD_POWER_OFF_MS)) {
        FSM_Transition(fsm, SDCARD_STATE_POWERING);
    }
}

static void powering_enter(FSM *fsm) {
    sdcard.boot_card_present = false;
    sdcard.error = HAL_SD_ERROR_NONE;
    sdcard.init_at = HAL_GetTick();
    HAL_GPIO_WritePin(SDMMC_PWR_Port, SDMMC_PWR_Pin, GPIO_PIN_SET);
    led_set(true);
    FSM_TransitionIn(fsm, SDCARD_STATE_INITIALISING, SDCARD_POWER_SETTLE_MS);
}

static void init_fail(uint32_t error) {
    sdcard.error = error;
    sdcard.handle.ErrorCode = error;
    FSM_Transition(&sdcard.fsm, SDCARD_STATE_ERROR);
}

static void initialising_enter(FSM *fsm) {
    (void)fsm;
    SDCard_InitStart(&sdcard.handle, &sdcard.info, sdcard.init_at);
}

static void initialising_service(FSM *fsm) {
    switch (SDCard_InitService()) {
        case SDCARD_INIT_READY:
            FSM_Transition(fsm, SDCARD_STATE_READY);
            break;
        case SDCARD_INIT_FAILED:
            init_fail(sdcard.handle.ErrorCode);
            break;
        case SDCARD_INIT_PENDING:
            break;
    }
}

static void peripheral_exit(FSM *fsm) {
    (void)fsm;
    SDMMC1->IDMACTRL = SDMMC_DISABLE_IDMA;
    SDCard_InitStop(&sdcard.handle);
    __DSB();
}

static void initialising_exit(FSM *fsm) {
    /* Successful initialisation transfers peripheral ownership to READY.
     * Failed initialisation or removal still needs host cleanup.
     * transition_id is the destination while the FSM runs exit callbacks.
     */
    if (fsm->transition_id != SDCARD_STATE_READY) {
        peripheral_exit(fsm);
    }
}

static void error_enter(FSM *fsm) {
    (void)fsm;
    power_off();
    sdcard.error_at = HAL_GetTick();
    led_set(true);
}

static void error_service(FSM *fsm) {
    (void)fsm;
    led_set(((HAL_GetTick() - sdcard.error_at) / SDCARD_ERROR_HALF_PERIOD_MS) % 2U == 0U);
}

static void ready_service(FSM *fsm) {
    (void)fsm;
    const uint32_t now = HAL_GetTick();
    if (sdcard.activity && (now - sdcard.activity_at >= SDCARD_ACTIVITY_MS)) {
        sdcard.activity = false;
    }
    /* Keep the blink phase independent of new transfers, so continuous I/O
     * alternates off/on rather than repeatedly extending the off pulse. */
    led_set(!sdcard.activity ||
            ((now - sdcard.activity_started_at) / SDCARD_ACTIVITY_MS) % 2U != 0U);
}

static const FSM_State states[] = {
    [SDCARD_STATE_OFF] = {
        .enter = off_enter, .service = off_service,
        .next_mask = FSM_NEXT(SDCARD_STATE_POWERING),
    },
    [SDCARD_STATE_POWERING] = {
        .enter = powering_enter,
        .next_mask = FSM_NEXT(SDCARD_STATE_INITIALISING) | FSM_NEXT(SDCARD_STATE_OFF) | FSM_NEXT(SDCARD_STATE_ERROR),
    },
    [SDCARD_STATE_INITIALISING] = {
        .enter = initialising_enter, .service = initialising_service,
        .exit = initialising_exit,
        .next_mask = FSM_NEXT(SDCARD_STATE_READY) | FSM_NEXT(SDCARD_STATE_ERROR) | FSM_NEXT(SDCARD_STATE_OFF),
    },
    [SDCARD_STATE_READY] = {
        .service = ready_service,
        .exit = peripheral_exit,
        .next_mask = FSM_NEXT(SDCARD_STATE_OFF) | FSM_NEXT(SDCARD_STATE_ERROR),
    },
    [SDCARD_STATE_ERROR] = {
        .enter = error_enter, .service = error_service,
        .next_mask = FSM_NEXT(SDCARD_STATE_OFF),
    },
};

static bool transaction_valid(const SDCard_Transaction *transaction) {
    if (transaction == NULL ||
        (transaction->operation != SDCARD_OPERATION_READ &&
         transaction->operation != SDCARD_OPERATION_WRITE)) {
        return false;
    }
    const uint32_t sector = transaction->sector;
    const uint32_t count = transaction->sector_count;
    /* Bound the multiplication by the host's data-length register first. */
    if (count == 0U || count > SDMMC_DLEN_DATALENGTH / SDCARD_SECTOR_SIZE) {
        return false;
    }
    const uint32_t bytes = count * SDCARD_SECTOR_SIZE;
    const uintptr_t address = (uintptr_t)transaction->data;
    const uintptr_t ram_end = SRAM3_BASE_NS + SRAM3_SIZE;
    return (address & 3U) == 0U && address >= SRAM1_BASE_NS &&
        address < ram_end && bytes <= ram_end - address &&
        sector < sdcard.handle.SdCard.LogBlockNbr &&
        count <= sdcard.handle.SdCard.LogBlockNbr - sector &&
        (sdcard.handle.SdCard.CardType != CARD_SDSC ||
         (sector <= UINT32_MAX / SDCARD_SECTOR_SIZE &&
          count - 1U <= UINT32_MAX / SDCARD_SECTOR_SIZE - sector));
}

bool SDCard_Queue(SDCard_Transaction *transaction) {
    if (SDCard_GetHandle() == NULL || !transaction_valid(transaction)) {
        return false;
    }
    transaction->queue_next = NULL;
    transaction->status = SDCARD_STATUS_PENDING;
    if (sdcard.queue_tail == NULL) {
        sdcard.queue_head = transaction;
    } else {
        sdcard.queue_tail->queue_next = transaction;
    }
    sdcard.queue_tail = transaction;
    return true;
}

static SDCard_Transaction *dequeue_transaction(void) {
    SDCard_Transaction *transaction = sdcard.queue_head;
    if (transaction != NULL) {
        sdcard.queue_head = transaction->queue_next;
        if (sdcard.queue_head == NULL) {
            sdcard.queue_tail = NULL;
        }
        transaction->queue_next = NULL;
    }
    return transaction;
}

static void transaction_complete(SDCard_Status status) {
    SDCard_Transaction *completed = sdcard.active_transaction;
    sdcard.active_transaction = NULL;
    sdcard.io_state = IO_IDLE;
    completed->queue_next = NULL;
    completed->status = status;
    SDCard_Transaction *next = completed->callback != NULL ? completed->callback(completed) : NULL;
    /* A callback can report a card error. Ignore follow-ons after shutdown,
     * and leave remaining queued transactions for the next service to cancel. */
    if (next != NULL && SDCard_GetHandle() != NULL) {
        next->queue_next = NULL;
        next->status = SDCARD_STATUS_PENDING;
        sdcard.active_transaction = next;
    }
}

static void transaction_cancel(SDCard_Transaction *transaction, SDCard_Status status) {
    transaction->queue_next = NULL;
    transaction->status = status;
    if (transaction->callback != NULL) {
        /* Shutdown never follows a callback's returned transaction. */
        (void)transaction->callback(transaction);
    }
}

static void queue_clear(void) {
    /* The lifecycle has already stopped/reset the host. Detach everything
     * before callbacks; no request survives into the next card session. */
    SDCard_Transaction *active = sdcard.active_transaction;
    SDCard_Transaction *queued = sdcard.queue_head;
    const bool unavailable = !SDCard_IsPresent();
    const SDCard_Status active_status = unavailable ? SDCARD_STATUS_UNAVAILABLE :
        (sdcard.io_state != IO_IDLE ? sdcard.failure_status : SDCARD_STATUS_CANCELLED);
    sdcard.active_transaction = NULL;
    sdcard.queue_head = NULL;
    sdcard.queue_tail = NULL;
    sdcard.io_state = IO_IDLE;
    if (active != NULL) {
        transaction_cancel(active, active_status);
    }
    while (queued != NULL) {
        SDCard_Transaction *next = queued->queue_next;
        transaction_cancel(queued, unavailable ? SDCARD_STATUS_UNAVAILABLE : SDCARD_STATUS_CANCELLED);
        queued = next;
    }
}

static void io_fail(uint32_t error) {
    sdcard.failure_status = (error & (HAL_SD_ERROR_TIMEOUT | HAL_SD_ERROR_CMD_RSP_TIMEOUT |
                                    HAL_SD_ERROR_DATA_TIMEOUT)) != 0U ?
        SDCARD_STATUS_TIMEOUT : SDCARD_STATUS_TRANSFER_ERROR;
    init_fail(error);
    FSM_Service(&sdcard.fsm);
    queue_clear();
}

static void io_command(uint32_t command, uint32_t argument) {
    SDMMC1->ICR = SDMMC_STATIC_CMD_FLAGS;
    const SDMMC_CmdInitTypeDef config = {
        .Argument = argument, .CmdIndex = command,
        .Response = SDMMC_RESPONSE_SHORT, .WaitForInterrupt = SDMMC_WAIT_NO,
        .CPSM = SDMMC_CPSM_ENABLE,
    };
    SDMMC_SendCommand(SDMMC1, &config);
}

static bool io_response(uint32_t command, uint32_t *error) {
    const uint32_t flags = SDMMC1->STA;
    *error = HAL_SD_ERROR_NONE;
    if ((flags & SDMMC_FLAG_CTIMEOUT) != 0U) {
        *error = HAL_SD_ERROR_CMD_RSP_TIMEOUT;
    } else if ((flags & SDMMC_FLAG_CCRCFAIL) != 0U) {
        *error = HAL_SD_ERROR_CMD_CRC_FAIL;
    } else if ((flags & SDMMC_FLAG_CMDACT) != 0U || (flags & SDMMC_FLAG_CMDREND) == 0U) {
        return false;
    } else {
        *error = SDMMC_GetCommandResponse(SDMMC1) == command ?
            SDCard_ResponseError(SDMMC1->RESP1) : HAL_SD_ERROR_CMD_CRC_FAIL;
    }
    SDMMC1->ICR = SDMMC_STATIC_CMD_FLAGS;
    return true;
}

static void sector_service(void) {
    if (SDCard_GetHandle() == NULL) {
        /* DET may change after the lifecycle's presence check this pass.
         * Apply shutdown before returning any buffer still owned by IDMA. */
        if (SDCard_GetState() == SDCARD_STATE_READY) {
            if (!SDCard_IsPresent()) {
                FSM_CancelTransition(&sdcard.fsm);
                FSM_Transition(&sdcard.fsm, SDCARD_STATE_OFF);
            }
            FSM_Service(&sdcard.fsm);
        }
        queue_clear();
        return;
    }
    if (sdcard.active_transaction == NULL) {
        sdcard.active_transaction = dequeue_transaction();
    }
    SDCard_Transaction *request = sdcard.active_transaction;
    if (request == NULL) {
        return;
    }
    if (sdcard.io_state == IO_IDLE && !transaction_valid(request)) {
        transaction_complete(SDCARD_STATUS_INVALID);
        return;
    }
    const bool write = request->operation == SDCARD_OPERATION_WRITE;
    const bool multiple = request->sector_count > 1U;
    const uint32_t command = write ?
        (multiple ? SDMMC_CMD_WRITE_MULT_BLOCK : SDMMC_CMD_WRITE_SINGLE_BLOCK) :
        (multiple ? SDMMC_CMD_READ_MULT_BLOCK : SDMMC_CMD_READ_SINGLE_BLOCK);
    if (sdcard.io_state == IO_IDLE) {
        sdcard.io_at = HAL_GetTick();
        sdcard.failure_status = SDCARD_STATUS_CANCELLED;
        sdcard.handle.State = HAL_SD_STATE_BUSY;
        sdcard.handle.ErrorCode = HAL_SD_ERROR_NONE;
        SDMMC1->MASK = 0U;
        SDMMC1->DCTRL = 0U;
        SDMMC1->ICR = SDMMC_STATIC_FLAGS;
        const SDMMC_DataInitTypeDef config = {
            .DataTimeOut = sdcard.info.bus_clock_hz,
            .DataLength = request->sector_count * SDCARD_SECTOR_SIZE,
            .DataBlockSize = SDMMC_DATABLOCK_SIZE_512B,
            .TransferDir = write ? SDMMC_TRANSFER_DIR_TO_CARD : SDMMC_TRANSFER_DIR_TO_SDMMC,
            .TransferMode = SDMMC_TRANSFER_MODE_BLOCK, .DPSM = SDMMC_DPSM_DISABLE,
        };
        SDMMC_ConfigData(SDMMC1, &config);
        __DMB();
        SDMMC1->IDMABASER = (uint32_t)request->data;
        SDMMC1->IDMACTRL = SDMMC_ENABLE_IDMA_SINGLE_BUFF;
        __SDMMC_CMDTRANS_ENABLE(SDMMC1);
        const uint32_t argument = sdcard.handle.SdCard.CardType == CARD_SDSC ?
            request->sector * SDCARD_SECTOR_SIZE : request->sector;
        io_command(command, argument);
        sdcard.io_state = IO_COMMAND;
        SDCard_Activity();
        return;
    }
    if (HAL_GetTick() - sdcard.io_at >= SDCARD_TRANSFER_TIMEOUT_MS) {
        io_fail(HAL_SD_ERROR_TIMEOUT);
        return;
    }
    if (sdcard.io_state == IO_COMMAND || sdcard.io_state == IO_DATA) {
        const uint32_t flags = SDMMC1->STA;
        uint32_t error = HAL_SD_ERROR_NONE;
        if ((flags & SDMMC_FLAG_IDMATE) != 0U) error |= HAL_SD_ERROR_DMA;
        if ((flags & SDMMC_FLAG_DTIMEOUT) != 0U) error |= HAL_SD_ERROR_DATA_TIMEOUT;
        if ((flags & SDMMC_FLAG_DCRCFAIL) != 0U) error |= HAL_SD_ERROR_DATA_CRC_FAIL;
        if ((flags & SDMMC_FLAG_RXOVERR) != 0U) error |= HAL_SD_ERROR_RX_OVERRUN;
        if ((flags & SDMMC_FLAG_TXUNDERR) != 0U) error |= HAL_SD_ERROR_TX_UNDERRUN;
        if (error != HAL_SD_ERROR_NONE) {
            io_fail(error);
            return;
        }
        if (sdcard.io_state == IO_COMMAND) {
            if (!io_response(command, &error)) return;
            if (error != HAL_SD_ERROR_NONE) {
                io_fail(error);
                return;
            }
            sdcard.io_state = IO_DATA;
        }
        /* A multiblock card keeps streaming until CMD12; do not wait for it
         * to stop on its own after the host's requested byte count arrives. */
        if ((flags & SDMMC_FLAG_DATAEND) == 0U ||
            (!multiple && (flags & SDMMC_FLAG_DPSMACT) != 0U)) return;
        if (SDMMC1->DCOUNT != 0U) {
            io_fail(HAL_SD_ERROR_GENERAL_UNKNOWN_ERR);
            return;
        }
        SDMMC1->IDMACTRL = SDMMC_DISABLE_IDMA;
        __SDMMC_CMDTRANS_DISABLE(SDMMC1);
        SDMMC1->DLEN = 0U;
        SDMMC1->DCTRL = 0U;
        SDMMC1->ICR = SDMMC_STATIC_DATA_FLAGS;
        __DMB();
        if (multiple) {
            __SDMMC_CMDSTOP_ENABLE(SDMMC1);
            io_command(SDMMC_CMD_STOP_TRANSMISSION, 0U);
            sdcard.io_state = IO_STOP;
            return;
        }
        sdcard.retry_at = HAL_GetTick();
        sdcard.io_state = IO_READY_WAIT;
    }
    if (sdcard.io_state == IO_STOP) {
        uint32_t error;
        if (!io_response(SDMMC_CMD_STOP_TRANSMISSION, &error)) return;
        __SDMMC_CMDSTOP_DISABLE(SDMMC1);
        SDMMC1->ICR = SDMMC_STATIC_DATA_FLAGS;
        /* Like HAL's stop helper, ignore the spurious range error a card
         * may report when stopping a transfer at its final sector. */
        if (error != HAL_SD_ERROR_NONE && error != HAL_SD_ERROR_ADDR_OUT_OF_RANGE) {
            io_fail(error);
            return;
        }
        sdcard.retry_at = HAL_GetTick();
        sdcard.io_state = IO_READY_WAIT;
    }
    if (sdcard.io_state == IO_READY_WAIT) {
        if ((SDMMC1->STA & SDMMC_FLAG_BUSYD0) != 0U || HAL_GetTick() - sdcard.retry_at < 1U) return;
        io_command(13U, sdcard.handle.SdCard.RelCardAdd << 16U);
        sdcard.io_state = IO_STATUS;
        return;
    }
    if (sdcard.io_state == IO_STATUS) {
        uint32_t error;
        if (!io_response(13U, &error)) return;
        if (error != HAL_SD_ERROR_NONE) {
            io_fail(error);
            return;
        }
        const uint32_t response = SDMMC1->RESP1;
        if ((response & (1UL << 8U)) == 0U ||
            ((response >> 9U) & 0xFU) != HAL_SD_CARD_TRANSFER ||
            (SDMMC1->STA & SDMMC_FLAG_BUSYD0) != 0U) {
            sdcard.retry_at = HAL_GetTick();
            sdcard.io_state = IO_READY_WAIT;
            return;
        }
        sdcard.handle.State = HAL_SD_STATE_READY;
        SDCard_Activity();
        transaction_complete(SDCARD_STATUS_SUCCESS);
    }
}

void SDCard_Init(void) {
    if (sdcard.fsm.states != NULL) {
        return;
    }
    HAL_GPIO_WritePin(SDMMC_PWR_Port, SDMMC_PWR_Pin, GPIO_PIN_RESET);
    led_set(false);
    GPIO_InitTypeDef gpio = {
        .Pin = SDMMC_PWR_Pin, .Mode = GPIO_MODE_OUTPUT_PP,
        .Pull = GPIO_NOPULL, .Speed = GPIO_SPEED_FREQ_LOW,
    };
    HAL_GPIO_Init(SDMMC_PWR_Port, &gpio);
    gpio.Pin = SDMMC_ACT_Pin;
    gpio.Mode = GPIO_MODE_OUTPUT_OD;
    HAL_GPIO_Init(SDMMC_ACT_Port, &gpio);
    gpio.Pin = SDMMC_DET_Pin;
    gpio.Mode = GPIO_MODE_INPUT;
    /* DET has an external pull-up on the PCB. */
    HAL_GPIO_Init(SDMMC_DET_Port, &gpio);
    /* Only a card already present at startup bypasses insertion debounce. */
    sdcard.boot_card_present = SDCard_IsPresent();
    FSM_Init(&sdcard.fsm, states, SDCARD_STATE_OFF, NULL);
    sdcard.fsm.transition_at = HAL_GetTick();
    FSM_Service(&sdcard.fsm);
}

void SDCard_Service(void) {
    if (sdcard.fsm.states == NULL) {
        return;
    }
    if (!SDCard_IsPresent() && (sdcard.fsm.current_id != SDCARD_STATE_OFF)) {
        /* Removal overrides the scheduled power-up transition immediately. */
        FSM_CancelTransition(&sdcard.fsm);
        FSM_Transition(&sdcard.fsm, SDCARD_STATE_OFF);
    }
    const SDCard_State state = SDCard_GetState();
    if ((state == SDCARD_STATE_POWERING || state == SDCARD_STATE_INITIALISING) && SDCard_IsPresent()) {
        const uint32_t elapsed = HAL_GetTick() - sdcard.init_at;
        led_set((elapsed / SDCARD_INIT_HALF_PERIOD_MS) % 2U == 0U);
        if (elapsed >= SDCARD_INIT_TIMEOUT_MS) {
            FSM_CancelTransition(&sdcard.fsm);
            init_fail(HAL_SD_ERROR_TIMEOUT);
        }
    }
    FSM_Service(&sdcard.fsm);
    sector_service();
}

SDCard_State SDCard_GetState(void) {
    return sdcard.fsm.states != NULL ? (SDCard_State)sdcard.fsm.current_id : SDCARD_STATE_OFF;
}

uint32_t SDCard_GetError(void) {
    return sdcard.error;
}

SD_HandleTypeDef *SDCard_GetHandle(void) {
    return (SDCard_GetState() == SDCARD_STATE_READY) &&
           !sdcard.fsm.transition_pending && SDCard_IsPresent() ? &sdcard.handle : NULL;
}

const SDCard_Info *SDCard_GetInfo(void) {
    return SDCard_GetHandle() != NULL ? &sdcard.info : NULL;
}

void SDCard_Activity(void) {
    if (SDCard_GetHandle() != NULL) {
        const uint32_t now = HAL_GetTick();
        if (!sdcard.activity || now - sdcard.activity_at >= SDCARD_ACTIVITY_MS) {
            sdcard.activity_started_at = now;
            led_set(false);
        }
        sdcard.activity_at = now;
        sdcard.activity = true;
    }
}

void SDCard_ReportError(uint32_t error) {
    if ((SDCard_GetHandle() != NULL) && FSM_Transition(&sdcard.fsm, SDCARD_STATE_ERROR)) {
        sdcard.error = error != HAL_SD_ERROR_NONE ? error : HAL_SD_ERROR_GENERAL_UNKNOWN_ERR;
        FSM_Service(&sdcard.fsm);
    }
}
