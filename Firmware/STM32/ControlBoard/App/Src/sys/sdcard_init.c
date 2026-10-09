#include "sys/sdcard_init.h"

#include <string.h>
#include "fsm/fsm.h"
#include "sys/gpio.h"

#define SDCARD_IO_TIMEOUT_MS 1000U
#define SDCARD_RETRY_MS 10U
#define SDCARD_INIT_CLOCK_HZ 400000U
#define SDCARD_DEFAULT_CLOCK_MAX_HZ 25000000U
#define SDCARD_HIGH_SPEED_CLOCK_MAX_HZ 50000000U
/* CMD6: leave groups 2..6 unchanged, request group 1 function 1. */
#define SDCARD_SPEED_CHECK_ARGUMENT 0x00FFFFF1U
#define SDCARD_SPEED_SWITCH_ARGUMENT 0x80FFFFF1U
#define SDCARD_READY_FOR_DATA (1UL << 8U)
#define SDCARD_APP_CMD (1UL << 5U)
#define SDCARD_OCR_READY (1UL << 31U)

/* Private negotiation states leave the public lifecycle API unchanged. */
typedef enum {
    INIT_HOST, INIT_CLOCKS, INIT_IDLE, INIT_INTERFACE, INIT_LEGACY_IDLE,
    INIT_OP_APP, INIT_OP_COND, INIT_OP_RETRY, INIT_CID, INIT_RCA,
    INIT_RCA_RETRY, INIT_CSD, INIT_SELECT, INIT_SELECT_BUSY,
    INIT_STATUS_LENGTH, INIT_STATUS_APP, INIT_STATUS_DATA,
    INIT_SCR_LENGTH, INIT_SCR_APP, INIT_SCR_DATA,
    INIT_WIDTH_APP, INIT_WIDTH, INIT_BLOCK_LENGTH,
    INIT_SPEED_SELECT, INIT_SPEED_CHECK, INIT_SPEED_RETRY, INIT_SPEED_SWITCH,
    INIT_SPEED_SETTLE, INIT_BUS,
    INIT_READY, INIT_READY_RETRY, INIT_STATE_COUNT,
} SDCard_InitState;
_Static_assert(INIT_STATE_COUNT <= FSM_MAX_STATES, "SD initialization FSM is too large");

typedef enum { RESPONSE_NONE, RESPONSE_R1, RESPONSE_R2, RESPONSE_R3,
               RESPONSE_R6, RESPONSE_R7 } SDCard_Response;

static struct {
    FSM init_fsm;
    uint32_t init_at;
    uint32_t io_at;
    uint32_t source_hz;
    uint32_t identification_hz;
    uint32_t response;
    uint32_t data[16];
    bool command_done;
    bool serviced;
    bool supports_switch;
    bool high_speed;
    SD_HandleTypeDef *handle;
    SDCard_Info *info;
    SDCard_InitResult result;
} sdcard_init;

void SDCard_InitDisconnectBus(void) {
    /* Disconnect host signals and internal pulls to avoid back-powering the card. */
    GPIO_InitTypeDef gpio = {
        .Pin = SDMMC_D0_Pin | SDMMC_D1_Pin | SDMMC_D2_Pin | SDMMC_D3_Pin | SDMMC_CK_Pin,
        .Mode = GPIO_MODE_ANALOG,
        .Pull = GPIO_NOPULL,
    };
    HAL_GPIO_Init(GPIOC, &gpio);
    gpio.Pin = SDMMC_CMD_Pin;
    HAL_GPIO_Init(SDMMC_CMD_Port, &gpio);
}

void HAL_SD_MspInit(SD_HandleTypeDef *handle) {
    if (handle->Instance != SDMMC1) {
        return;
    }

    __HAL_RCC_SDMMC1_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {
        .Pin = SDMMC_D0_Pin | SDMMC_D1_Pin | SDMMC_D2_Pin | SDMMC_D3_Pin,
        .Mode = GPIO_MODE_AF_PP,
        .Pull = GPIO_PULLUP,
        .Speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .Alternate = GPIO_AF12_SDMMC1,
    };
    HAL_GPIO_Init(GPIOC, &gpio);
    gpio.Pin = SDMMC_CMD_Pin;
    HAL_GPIO_Init(SDMMC_CMD_Port, &gpio);
    gpio.Pin = SDMMC_CK_Pin;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(SDMMC_CK_Port, &gpio);
}

void HAL_SD_MspDeInit(SD_HandleTypeDef *handle) {
    if (handle->Instance != SDMMC1) {
        return;
    }
    __HAL_RCC_SDMMC1_FORCE_RESET();
    __HAL_RCC_SDMMC1_RELEASE_RESET();
    SDCard_InitDisconnectBus();
    __HAL_RCC_SDMMC1_CLK_DISABLE();
}

static void init_fail(uint32_t error) {
    sdcard_init.handle->ErrorCode = error;
    sdcard_init.result = SDCARD_INIT_FAILED;
}

/* FSM_Service can enter several states in one call. Only service one private
 * state per outer pass, even when a response is already available. */
static bool init_service_once(FSM *fsm) {
    if (sdcard_init.serviced || fsm->transition_pending || sdcard_init.result != SDCARD_INIT_PENDING) {
        return false;
    }
    sdcard_init.serviced = true;
    return true;
}

static uint32_t rca_argument(void) {
    return sdcard_init.handle->SdCard.RelCardAdd << 16U;
}

static uint32_t operating_argument(void) {
    /* Advertise only this board's 3.3V window, never 1.8V switching. */
    return (SDMMC_VOLTAGE_WINDOW_SD & ~SDCARD_OCR_READY) |
           (sdcard_init.handle->SdCard.CardVersion == CARD_V2_X ? SDMMC_HIGH_CAPACITY : 0U);
}

uint32_t SDCard_ResponseError(uint32_t response) {
    static const struct { uint32_t bit; uint32_t error; } errors[] = {
        {SDMMC_CARD_LOCKED, HAL_SD_ERROR_LOCK_UNLOCK_FAILED},
        {SDMMC_OCR_ADDR_OUT_OF_RANGE, HAL_SD_ERROR_ADDR_OUT_OF_RANGE},
        {SDMMC_OCR_ADDR_MISALIGNED, HAL_SD_ERROR_ADDR_MISALIGNED},
        {SDMMC_OCR_BLOCK_LEN_ERR, HAL_SD_ERROR_BLOCK_LEN_ERR},
        {SDMMC_OCR_ERASE_SEQ_ERR, HAL_SD_ERROR_ERASE_SEQ_ERR},
        {SDMMC_OCR_BAD_ERASE_PARAM, HAL_SD_ERROR_BAD_ERASE_PARAM},
        {SDMMC_OCR_WRITE_PROT_VIOLATION, HAL_SD_ERROR_WRITE_PROT_VIOLATION},
        {SDMMC_OCR_LOCK_UNLOCK_FAILED, HAL_SD_ERROR_LOCK_UNLOCK_FAILED},
        {SDMMC_OCR_COM_CRC_FAILED, HAL_SD_ERROR_COM_CRC_FAILED},
        {SDMMC_OCR_ILLEGAL_CMD, HAL_SD_ERROR_ILLEGAL_CMD},
        {SDMMC_OCR_CARD_ECC_FAILED, HAL_SD_ERROR_CARD_ECC_FAILED},
        {SDMMC_OCR_CC_ERROR, HAL_SD_ERROR_CC_ERR},
        {SDMMC_OCR_GENERAL_UNKNOWN_ERROR, HAL_SD_ERROR_GENERAL_UNKNOWN_ERR},
        {SDMMC_OCR_STREAM_READ_UNDERRUN, HAL_SD_ERROR_STREAM_READ_UNDERRUN},
        {SDMMC_OCR_STREAM_WRITE_OVERRUN, HAL_SD_ERROR_STREAM_WRITE_OVERRUN},
        {SDMMC_OCR_CID_CSD_OVERWRITE, HAL_SD_ERROR_CID_CSD_OVERWRITE},
        {SDMMC_OCR_WP_ERASE_SKIP, HAL_SD_ERROR_WP_ERASE_SKIP},
        {SDMMC_OCR_CARD_ECC_DISABLED, HAL_SD_ERROR_CARD_ECC_DISABLED},
        {SDMMC_OCR_ERASE_RESET, HAL_SD_ERROR_ERASE_RESET},
        {SDMMC_OCR_AKE_SEQ_ERROR, HAL_SD_ERROR_AKE_SEQ_ERR},
    };
    for (uint32_t i = 0U; i < sizeof(errors) / sizeof(errors[0]); ++i) {
        if ((response & errors[i].bit) != 0U) {
            return errors[i].error;
        }
    }
    return HAL_SD_ERROR_NONE;
}

static void interface_complete(FSM *fsm) {
    if ((sdcard_init.response & 0xFFFU) != SDMMC_CHECK_PATTERN) {
        init_fail(HAL_SD_ERROR_INVALID_VOLTRANGE);
        return;
    }
    sdcard_init.handle->SdCard.CardVersion = CARD_V2_X;
    SDCard_InitState next = INIT_OP_APP;
    FSM_Transition(fsm, next);
}

static void operating_complete(FSM *fsm) {
    if ((sdcard_init.response & (SDMMC_VOLTAGE_WINDOW_SD & ~SDCARD_OCR_READY)) == 0U) {
        init_fail(HAL_SD_ERROR_INVALID_VOLTRANGE);
    } else if ((sdcard_init.response & SDCARD_OCR_READY) == 0U) {
        SDCard_InitState next = INIT_OP_RETRY;
        FSM_Transition(fsm, next);
    } else {
        sdcard_init.handle->SdCard.CardType =
            sdcard_init.handle->SdCard.CardVersion == CARD_V2_X &&
            (sdcard_init.response & SDMMC_HIGH_CAPACITY) != 0U ? CARD_SDHC_SDXC : CARD_SDSC;
        SDCard_InitState next = INIT_CID;
        FSM_Transition(fsm, next);
    }
}

static void cid_complete(FSM *fsm) {
    sdcard_init.handle->CID[0] = SDMMC1->RESP1;
    sdcard_init.handle->CID[1] = SDMMC1->RESP2;
    sdcard_init.handle->CID[2] = SDMMC1->RESP3;
    sdcard_init.handle->CID[3] = SDMMC1->RESP4;
    SDCard_InitState next = INIT_RCA;
    FSM_Transition(fsm, next);
}

static void rca_complete(FSM *fsm) {
    sdcard_init.handle->SdCard.RelCardAdd = sdcard_init.response >> 16U;
    SDCard_InitState next = sdcard_init.handle->SdCard.RelCardAdd == 0U ? INIT_RCA_RETRY : INIT_CSD;
    FSM_Transition(fsm, next);
}

/* CSD TRAN_SPEED encodes a decimal multiplier and a frequency unit.
 * This is the limit for the current default-speed timing mode; the HAL speed
 * category from SD Status does not negotiate CMD6 high-speed timing.
 */
static uint32_t default_clock_limit(uint8_t tran_speed) {
    static const uint32_t units_hz[] = {100000U, 1000000U, 10000000U, 100000000U};
    static const uint8_t multipliers_tenths[] = {
        0U, 10U, 12U, 13U, 15U, 20U, 25U, 30U,
        35U, 40U, 45U, 50U, 55U, 60U, 70U, 80U,
    };
    const uint32_t unit = tran_speed & 7U;
    const uint32_t multiplier = multipliers_tenths[(tran_speed >> 3U) & 0xFU];
    if ((tran_speed & 0x80U) != 0U || unit >= sizeof(units_hz) / sizeof(units_hz[0]) || multiplier == 0U) {
        return 0U;
    }
    const uint32_t limit = (units_hz[unit] / 10U) * multiplier;
    return limit < SDCARD_DEFAULT_CLOCK_MAX_HZ ? limit : SDCARD_DEFAULT_CLOCK_MAX_HZ;
}

static uint32_t clock_divider(uint32_t limit_hz) {
    return sdcard_init.source_hz <= limit_hz ? 0U :
        (sdcard_init.source_hz + 2U * limit_hz - 1U) / (2U * limit_hz);
}

static void csd_complete(FSM *fsm) {
    HAL_SD_CardCSDTypeDef csd;
    sdcard_init.handle->CSD[0] = SDMMC1->RESP1;
    sdcard_init.handle->CSD[1] = SDMMC1->RESP2;
    sdcard_init.handle->CSD[2] = SDMMC1->RESP3;
    sdcard_init.handle->CSD[3] = SDMMC1->RESP4;
    sdcard_init.handle->SdCard.Class = (SDMMC1->RESP2 >> 20U) & 0xFFFU;
    const uint32_t structure = sdcard_init.handle->CSD[0] >> 30U;
    if (structure != (sdcard_init.handle->SdCard.CardType == CARD_SDSC ? 0U : 1U) ||
        HAL_SD_GetCardCSD(sdcard_init.handle, &csd) != HAL_OK ||
        sdcard_init.handle->SdCard.LogBlockNbr == 0U) {
        init_fail(HAL_SD_ERROR_UNSUPPORTED_FEATURE);
        return;
    }
    const uint32_t limit_hz = default_clock_limit(csd.MaxBusClkFrec);
    if (limit_hz == 0U) {
        init_fail(HAL_SD_ERROR_UNSUPPORTED_FEATURE);
        return;
    }
    /* Divider zero bypasses division. Otherwise round UP so the bus clock
     * never exceeds the card's advertised rate, even for older slower cards.
     * Keep identification clock applied until INIT_BUS finishes negotiation.
     */
    sdcard_init.handle->Init.ClockDiv = clock_divider(limit_hz);
    SDCard_InitState next = INIT_SELECT;
    FSM_Transition(fsm, next);
}

static void status_complete(FSM *fsm) {
    /* Match HAL's status classification, without enabling a faster mode. */
    sdcard_init.handle->SdCard.CardSpeed = sdcard_init.handle->SdCard.CardType == CARD_SDSC ?
        CARD_NORMAL_SPEED : ((sdcard_init.data[3] & 0xFFU) != 0U ?
                             CARD_ULTRA_HIGH_SPEED : CARD_HIGH_SPEED);
    SDCard_InitState next = INIT_SCR_LENGTH;
    FSM_Transition(fsm, next);
}

static void scr_complete(FSM *fsm) {
    const uint32_t scr = __REV(sdcard_init.data[0]);
    if ((scr >> 28U) != 0U || (scr & SDMMC_SINGLE_BUS_SUPPORT) == 0U) {
        init_fail(HAL_SD_ERROR_UNSUPPORTED_FEATURE);
        return;
    }
    /* CMD6 starts with SD 1.10 and requires command class 10. SDSC cards
     * can support it too; the SD Status speed category is not sufficient. */
    sdcard_init.supports_switch = ((scr >> 24U) & 0xFU) >= 1U &&
                            (sdcard_init.handle->SdCard.Class & (1UL << 10U)) != 0U;
    SDCard_InitState next = (scr & SDMMC_WIDE_BUS_SUPPORT) != 0U ? INIT_WIDTH_APP : INIT_BLOCK_LENGTH;
    FSM_Transition(fsm, next);
}

static void width_complete(FSM *fsm) {
    sdcard_init.handle->Init.BusWide = SDMMC_BUS_WIDE_4B;
    /* Match the host immediately after the successful ACMD6 response. */
    MODIFY_REG(SDMMC1->CLKCR, SDMMC_CLKCR_WIDBUS, SDMMC_BUS_WIDE_4B);
    SDCard_InitState next = INIT_BLOCK_LENGTH;
    FSM_Transition(fsm, next);
}

/* CMD6 packets arrive in wire byte order in the IDMA buffer. Group 1
 * support is bits 415:400, selection 379:376, and busy 287:272.
 * Busy fields are valid only for switch status version 1 or later.
 */
static void speed_check_complete(FSM *fsm) {
    const uint8_t *status = (const uint8_t *)sdcard_init.data;
    SDCard_InitState next;
    if ((status[13] & 2U) == 0U) {
        next = INIT_BUS;
    } else if (status[17] != 0U && (status[29] & 2U) != 0U) {
        next = INIT_SPEED_RETRY;
    } else if ((status[16] & 0xFU) == 0xFU) {
        next = INIT_BUS;
    } else {
        next = INIT_SPEED_SWITCH;
    }
    /* Allow >=8 identification clocks after the status packet before
     * another switch command. The extra tick covers tick quantisation. */
    const uint32_t delay_ms = 1U + (8000U + sdcard_init.identification_hz - 1U) / sdcard_init.identification_hz;
    FSM_TransitionIn(fsm, next, delay_ms);
}

static void speed_switch_complete(FSM *fsm) {
    const uint8_t *status = (const uint8_t *)sdcard_init.data;
    const uint32_t selected = status[16] & 0xFU;
    if (selected == 1U) {
        sdcard_init.high_speed = true;
        sdcard_init.handle->Init.ClockDiv = clock_divider(SDCARD_HIGH_SPEED_CLOCK_MAX_HZ);
    } else if (selected != 0U && selected != 0xFU) {
        /* Do not continue with an unexpected bus timing selection. */
        init_fail(HAL_SD_ERROR_UNSUPPORTED_FEATURE);
        return;
    }
    /* A rejected switch leaves the CSD-derived default divider in place. */
    SDCard_InitState next = INIT_SPEED_SETTLE;
    FSM_Transition(fsm, next);
}

static void ready_complete(FSM *fsm) {
    if ((sdcard_init.response & SDCARD_READY_FOR_DATA) == 0U ||
        ((sdcard_init.response >> 9U) & 0xFU) != HAL_SD_CARD_TRANSFER ||
        (SDMMC1->STA & SDMMC_FLAG_BUSYD0) != 0U) {
        SDCard_InitState next = INIT_READY_RETRY;
        FSM_Transition(fsm, next);
        return;
    }
    if (HAL_GetTick() - sdcard_init.init_at >= SDCARD_INIT_TIMEOUT_MS) {
        init_fail(HAL_SD_ERROR_TIMEOUT);
        return;
    }
    const uint32_t divider = SDMMC1->CLKCR & SDMMC_CLKCR_CLKDIV;
    sdcard_init.info->bus_clock_hz = divider == 0U ? sdcard_init.source_hz : sdcard_init.source_hz / (2U * divider);
    sdcard_init.info->bus_width_bits = sdcard_init.handle->Init.BusWide == SDMMC_BUS_WIDE_4B ? 4U : 1U;
    sdcard_init.info->card_type = sdcard_init.handle->SdCard.CardType;
    sdcard_init.info->card_speed = sdcard_init.handle->SdCard.CardSpeed;
    sdcard_init.info->high_speed = sdcard_init.high_speed;
    sdcard_init.handle->ErrorCode = HAL_SD_ERROR_NONE;
    sdcard_init.handle->Context = SD_CONTEXT_NONE;
    sdcard_init.handle->State = HAL_SD_STATE_READY;
    sdcard_init.result = SDCARD_INIT_READY;
}

/* Commands are described per FSM state; callbacks handle only the response
 * specific to that phase. No polling HAL command wrappers are used. */
typedef struct {
    uint32_t command;
    SDCard_Response response;
    uint32_t argument;
    uint32_t (*get_argument)(void);
    uint32_t data_bytes;
    SDCard_InitState next;
    void (*complete)(FSM *fsm);
} SDCard_InitCommand;

static const SDCard_InitCommand init_commands[INIT_STATE_COUNT] = {
    [INIT_IDLE] = {0U, RESPONSE_NONE, 0U, NULL, 0U, INIT_INTERFACE, NULL},
    [INIT_INTERFACE] = {8U, RESPONSE_R7, SDMMC_CHECK_PATTERN, NULL, 0U, INIT_OP_APP, interface_complete},
    [INIT_LEGACY_IDLE] = {0U, RESPONSE_NONE, 0U, NULL, 0U, INIT_OP_APP, NULL},
    [INIT_OP_APP] = {55U, RESPONSE_R1, 0U, NULL, 0U, INIT_OP_COND, NULL},
    [INIT_OP_COND] = {41U, RESPONSE_R3, 0U, operating_argument, 0U, INIT_CID, operating_complete},
    [INIT_CID] = {2U, RESPONSE_R2, 0U, NULL, 0U, INIT_RCA, cid_complete},
    [INIT_RCA] = {3U, RESPONSE_R6, 0U, NULL, 0U, INIT_CSD, rca_complete},
    [INIT_CSD] = {9U, RESPONSE_R2, 0U, rca_argument, 0U, INIT_SELECT, csd_complete},
    [INIT_SELECT] = {7U, RESPONSE_R1, 0U, rca_argument, 0U, INIT_SELECT_BUSY, NULL},
    [INIT_STATUS_LENGTH] = {16U, RESPONSE_R1, 64U, NULL, 0U, INIT_STATUS_APP, NULL},
    [INIT_STATUS_APP] = {55U, RESPONSE_R1, 0U, rca_argument, 0U, INIT_STATUS_DATA, NULL},
    [INIT_STATUS_DATA] = {13U, RESPONSE_R1, 0U, NULL, 64U, INIT_SCR_LENGTH, status_complete},
    [INIT_SCR_LENGTH] = {16U, RESPONSE_R1, 8U, NULL, 0U, INIT_SCR_APP, NULL},
    [INIT_SCR_APP] = {55U, RESPONSE_R1, 0U, rca_argument, 0U, INIT_SCR_DATA, NULL},
    [INIT_SCR_DATA] = {51U, RESPONSE_R1, 0U, NULL, 8U, INIT_WIDTH_APP, scr_complete},
    [INIT_WIDTH_APP] = {55U, RESPONSE_R1, 0U, rca_argument, 0U, INIT_WIDTH, NULL},
    [INIT_WIDTH] = {6U, RESPONSE_R1, 2U, NULL, 0U, INIT_BLOCK_LENGTH, width_complete},
    [INIT_BLOCK_LENGTH] = {16U, RESPONSE_R1, 512U, NULL, 0U, INIT_SPEED_SELECT, NULL},
    [INIT_SPEED_CHECK] = {6U, RESPONSE_R1, SDCARD_SPEED_CHECK_ARGUMENT, NULL, 64U, INIT_BUS, speed_check_complete},
    [INIT_SPEED_SWITCH] = {6U, RESPONSE_R1, SDCARD_SPEED_SWITCH_ARGUMENT, NULL, 64U, INIT_SPEED_SETTLE, speed_switch_complete},
    [INIT_READY] = {13U, RESPONSE_R1, 0U, rca_argument, 0U, INIT_READY_RETRY, ready_complete},
};

static void command_enter(FSM *fsm) {
    const SDCard_InitCommand *step = &init_commands[fsm->current_id];
    sdcard_init.io_at = HAL_GetTick();
    sdcard_init.command_done = false;
    SDMMC1->ICR = SDMMC_STATIC_CMD_FLAGS;
    __SDMMC_CMDTRANS_DISABLE(SDMMC1);
    if (step->data_bytes != 0U) {
        SDMMC1->DCTRL = 0U;
        SDMMC1->ICR = SDMMC_STATIC_DATA_FLAGS;
        memset(sdcard_init.data, 0, sizeof(sdcard_init.data));
        const SDMMC_DataInitTypeDef config = {
            .DataTimeOut = sdcard_init.identification_hz,
            .DataLength = step->data_bytes,
            .DataBlockSize = step->data_bytes == 8U ? SDMMC_DATABLOCK_SIZE_8B : SDMMC_DATABLOCK_SIZE_64B,
            .TransferDir = SDMMC_TRANSFER_DIR_TO_SDMMC,
            .TransferMode = SDMMC_TRANSFER_MODE_BLOCK,
            .DPSM = SDMMC_DPSM_DISABLE,
        };
        SDMMC_ConfigData(SDMMC1, &config);
        /* FIFO access is valid only while DPSMACT is set. A main-loop pass
         * can arrive after an entire status/SCR packet, so let internal DMA
         * receive into our word-aligned SRAM buffer independently of service
         * timing. Single-buffer mode uses DLEN (64 or 8 bytes) as its bound.
         * CMDTRANS starts reception with the data command, as in HAL DMA reads.
         */
        __DMB();
        SDMMC1->IDMABASER = (uint32_t)sdcard_init.data;
        SDMMC1->IDMACTRL = SDMMC_ENABLE_IDMA_SINGLE_BUFF;
        __SDMMC_CMDTRANS_ENABLE(SDMMC1);
    }
    const SDMMC_CmdInitTypeDef command = {
        .Argument = step->get_argument != NULL ? step->get_argument() : step->argument,
        .CmdIndex = step->command,
        .Response = step->response == RESPONSE_NONE ? SDMMC_RESPONSE_NO :
                    (step->response == RESPONSE_R2 ? SDMMC_RESPONSE_LONG : SDMMC_RESPONSE_SHORT),
        .WaitForInterrupt = SDMMC_WAIT_NO,
        .CPSM = SDMMC_CPSM_ENABLE,
    };
    SDMMC_SendCommand(SDMMC1, &command);
}

/* Return false when still pending. A completed command reports its error
 * separately, allowing CMD8 response timeout to take the legacy-card path. */
static bool command_poll(const SDCard_InitCommand *step, uint32_t *error) {
    const uint32_t status = SDMMC1->STA;
    *error = HAL_SD_ERROR_NONE;
    if ((status & SDMMC_FLAG_CTIMEOUT) != 0U) {
        *error = HAL_SD_ERROR_CMD_RSP_TIMEOUT;
    } else if ((status & SDMMC_FLAG_CCRCFAIL) != 0U && step->response != RESPONSE_R3) {
        *error = HAL_SD_ERROR_CMD_CRC_FAIL;
    } else if ((status & SDMMC_FLAG_CMDACT) != 0U ||
               (status & (step->response == RESPONSE_NONE ? SDMMC_FLAG_CMDSENT :
                          SDMMC_FLAG_CMDREND | SDMMC_FLAG_CCRCFAIL)) == 0U) {
        if (HAL_GetTick() - sdcard_init.io_at < SDCARD_IO_TIMEOUT_MS) {
            return false;
        }
        *error = HAL_SD_ERROR_TIMEOUT;
    } else {
        sdcard_init.response = SDMMC1->RESP1;
        if (step->response != RESPONSE_NONE && step->response != RESPONSE_R2 &&
            step->response != RESPONSE_R3 && SDMMC_GetCommandResponse(SDMMC1) != step->command) {
            *error = HAL_SD_ERROR_CMD_CRC_FAIL;
        } else if (step->response == RESPONSE_R1) {
            *error = SDCard_ResponseError(sdcard_init.response);
            if (*error == HAL_SD_ERROR_NONE && step->command == 55U &&
                (sdcard_init.response & SDCARD_APP_CMD) == 0U) {
                *error = HAL_SD_ERROR_UNSUPPORTED_FEATURE;
            }
        } else if (step->response == RESPONSE_R6) {
            if ((sdcard_init.response & SDMMC_R6_ILLEGAL_CMD) != 0U) {
                *error = HAL_SD_ERROR_ILLEGAL_CMD;
            } else if ((sdcard_init.response & SDMMC_R6_COM_CRC_FAILED) != 0U) {
                *error = HAL_SD_ERROR_COM_CRC_FAILED;
            } else if ((sdcard_init.response & SDMMC_R6_GENERAL_UNKNOWN_ERROR) != 0U) {
                *error = HAL_SD_ERROR_GENERAL_UNKNOWN_ERR;
            }
        }
    }
    SDMMC1->ICR = SDMMC_STATIC_CMD_FLAGS;
    return true;
}

static void command_service(FSM *fsm) {
    const SDCard_InitCommand *step = &init_commands[fsm->current_id];
    uint32_t error;
    if (!sdcard_init.command_done) {
        if (!command_poll(step, &error)) {
            return;
        }
        if (error != HAL_SD_ERROR_NONE) {
            if (fsm->current_id == INIT_INTERFACE && error == HAL_SD_ERROR_CMD_RSP_TIMEOUT) {
                sdcard_init.handle->SdCard.CardVersion = CARD_V1_X;
                SDCard_InitState next = INIT_LEGACY_IDLE;
                FSM_Transition(fsm, next);
            } else if (fsm->current_id == INIT_SPEED_CHECK && error == HAL_SD_ERROR_ILLEGAL_CMD &&
                       (SDMMC1->STA & SDMMC_FLAG_DPSMACT) == 0U) {
                /* Some older cards reject the optional probe. No data
                 * transfer started, so disarm reception and retain default. */
                SDMMC1->IDMACTRL = SDMMC_DISABLE_IDMA;
                __SDMMC_CMDTRANS_DISABLE(SDMMC1);
                SDMMC1->DCTRL = 0U;
                SDMMC1->ICR = SDMMC_STATIC_DATA_FLAGS;
                SDCard_InitState next = INIT_BUS;
                FSM_Transition(fsm, next);
            } else {
                init_fail(error);
            }
            return;
        }
        sdcard_init.command_done = true;
    }
    if (step->data_bytes != 0U) {
        const uint32_t status = SDMMC1->STA;
        if ((status & SDMMC_FLAG_IDMATE) != 0U) {
            init_fail(HAL_SD_ERROR_DMA);
            return;
        }
        if ((status & SDMMC_FLAG_DTIMEOUT) != 0U) {
            init_fail(HAL_SD_ERROR_DATA_TIMEOUT);
            return;
        }
        if ((status & SDMMC_FLAG_DCRCFAIL) != 0U) {
            init_fail(HAL_SD_ERROR_DATA_CRC_FAIL);
            return;
        }
        if ((status & SDMMC_FLAG_RXOVERR) != 0U) {
            init_fail(HAL_SD_ERROR_RX_OVERRUN);
            return;
        }
        if ((status & SDMMC_FLAG_DATAEND) == 0U ||
            (status & SDMMC_FLAG_DPSMACT) != 0U) {
            if (HAL_GetTick() - sdcard_init.io_at >= SDCARD_IO_TIMEOUT_MS) {
                init_fail(HAL_SD_ERROR_DATA_TIMEOUT);
            }
            return;
        }
        if (SDMMC1->DCOUNT != 0U) {
            init_fail(HAL_SD_ERROR_GENERAL_UNKNOWN_ERR);
            return;
        }
        SDMMC1->IDMACTRL = SDMMC_DISABLE_IDMA;
        __SDMMC_CMDTRANS_DISABLE(SDMMC1);
        SDMMC1->DCTRL = 0U;
        SDMMC1->ICR = SDMMC_STATIC_DATA_FLAGS;
        /* DATAEND includes IDMA completion; make received SRAM data visible
         * before the next state's status/SCR decoder reads it. */
        __DMB();
    }
    if (step->complete != NULL) {
        step->complete(fsm);
    } else {
        SDCard_InitState next = step->next;
        FSM_Transition(fsm, next);
    }
}

static void host_service(FSM *fsm) {
    __HAL_RCC_SDMMC1_CONFIG(RCC_SDMMC1CLKSOURCE_PLL2R);
    memset(sdcard_init.handle, 0, sizeof(*sdcard_init.handle));
    sdcard_init.supports_switch = false;
    sdcard_init.high_speed = false;
    sdcard_init.handle->Instance = SDMMC1;
    sdcard_init.handle->Lock = HAL_UNLOCKED;
    sdcard_init.handle->State = HAL_SD_STATE_PROGRAMMING;
    sdcard_init.handle->Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;
    sdcard_init.handle->Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;
    sdcard_init.handle->Init.BusWide = SDMMC_BUS_WIDE_1B;
    sdcard_init.handle->Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;
    sdcard_init.handle->Init.ClockDiv = 1U; /* Replaced with the CSD-derived divider before INIT_BUS. */
    HAL_SD_MspInit(sdcard_init.handle);
    __HAL_RCC_SDMMC1_FORCE_RESET();
    __HAL_RCC_SDMMC1_RELEASE_RESET();
    sdcard_init.source_hz = HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_SDMMC1);
    if (sdcard_init.source_hz == 0U) {
        init_fail(SDMMC_ERROR_INVALID_PARAMETER);
        return;
    }
    SDMMC_InitTypeDef init = sdcard_init.handle->Init;
    init.ClockDiv = sdcard_init.source_hz <= SDCARD_INIT_CLOCK_HZ ? 0U :
        (sdcard_init.source_hz + 2U * SDCARD_INIT_CLOCK_HZ - 1U) / (2U * SDCARD_INIT_CLOCK_HZ);
    sdcard_init.identification_hz = init.ClockDiv == 0U ? sdcard_init.source_hz :
        sdcard_init.source_hz / (2U * init.ClockDiv);
    SDMMC_Init(SDMMC1, init);
    SDMMC_PowerState_ON(SDMMC1);
    sdcard_init.io_at = HAL_GetTick();
    SDCard_InitState next = INIT_CLOCKS;
    FSM_Transition(fsm, next);
}

static void clocks_service(FSM *fsm) {
    const uint32_t delay_ms = 1U + (74000U + sdcard_init.identification_hz - 1U) / sdcard_init.identification_hz;
    if (HAL_GetTick() - sdcard_init.io_at >= delay_ms) {
        SDCard_InitState next = INIT_IDLE;
        FSM_Transition(fsm, next);
    }
}

static void select_busy_enter(FSM *fsm) {
    (void)fsm;
    sdcard_init.io_at = HAL_GetTick();
}

static void select_busy_service(FSM *fsm) {
    if ((SDMMC1->STA & SDMMC_FLAG_BUSYD0) == 0U) {
        SDCard_InitState next = INIT_STATUS_LENGTH;
        FSM_Transition(fsm, next);
    } else if (HAL_GetTick() - sdcard_init.io_at >= SDCARD_IO_TIMEOUT_MS) {
        init_fail(HAL_SD_ERROR_TIMEOUT);
    }
}

static void op_retry_enter(FSM *fsm) {
    FSM_TransitionIn(fsm, INIT_OP_APP, SDCARD_RETRY_MS);
}

static void rca_retry_enter(FSM *fsm) {
    FSM_TransitionIn(fsm, INIT_RCA, SDCARD_RETRY_MS);
}

static void ready_retry_enter(FSM *fsm) {
    FSM_TransitionIn(fsm, INIT_READY, SDCARD_RETRY_MS);
}

static void speed_select_service(FSM *fsm) {
    SDCard_InitState next = sdcard_init.supports_switch ? INIT_SPEED_CHECK : INIT_BUS;
    FSM_Transition(fsm, next);
}

static void speed_retry_enter(FSM *fsm) {
    FSM_TransitionIn(fsm, INIT_SPEED_CHECK, SDCARD_RETRY_MS);
}

static void speed_settle_enter(FSM *fsm) {
    /* The card needs >=8 clocks at the old rate after CMD6 before the
     * host increases the clock. Wait asynchronously, including a spare tick. */
    const uint32_t delay_ms = 1U + (8000U + sdcard_init.identification_hz - 1U) / sdcard_init.identification_hz;
    FSM_TransitionIn(fsm, INIT_BUS, delay_ms);
}

static void bus_service(FSM *fsm) {
    SDMMC_Init(SDMMC1, sdcard_init.handle->Init);
    SDCard_InitState next = INIT_READY;
    FSM_Transition(fsm, next);
}

#define INIT_COMMAND_STATE(next_states) { .enter = command_enter, .service = command_service, \
    .service_predicate = init_service_once, .next_mask = (next_states) }
#define INIT_SERVICE_STATE(callback, next_states) { .service = (callback), \
    .service_predicate = init_service_once, .next_mask = (next_states) }
static const FSM_State init_states[INIT_STATE_COUNT] = {
    [INIT_HOST] = INIT_SERVICE_STATE(host_service, FSM_NEXT(INIT_CLOCKS)),
    [INIT_CLOCKS] = INIT_SERVICE_STATE(clocks_service, FSM_NEXT(INIT_IDLE)),
    [INIT_IDLE] = INIT_COMMAND_STATE(FSM_NEXT(INIT_INTERFACE)),
    [INIT_INTERFACE] = INIT_COMMAND_STATE(FSM_NEXT(INIT_OP_APP) | FSM_NEXT(INIT_LEGACY_IDLE)),
    [INIT_LEGACY_IDLE] = INIT_COMMAND_STATE(FSM_NEXT(INIT_OP_APP)),
    [INIT_OP_APP] = INIT_COMMAND_STATE(FSM_NEXT(INIT_OP_COND)),
    [INIT_OP_COND] = INIT_COMMAND_STATE(FSM_NEXT(INIT_OP_RETRY) | FSM_NEXT(INIT_CID)),
    [INIT_OP_RETRY] = { .enter = op_retry_enter, .next_mask = FSM_NEXT(INIT_OP_APP) },
    [INIT_CID] = INIT_COMMAND_STATE(FSM_NEXT(INIT_RCA)),
    [INIT_RCA] = INIT_COMMAND_STATE(FSM_NEXT(INIT_RCA_RETRY) | FSM_NEXT(INIT_CSD)),
    [INIT_RCA_RETRY] = { .enter = rca_retry_enter, .next_mask = FSM_NEXT(INIT_RCA) },
    [INIT_CSD] = INIT_COMMAND_STATE(FSM_NEXT(INIT_SELECT)),
    [INIT_SELECT] = INIT_COMMAND_STATE(FSM_NEXT(INIT_SELECT_BUSY)),
    [INIT_SELECT_BUSY] = { .enter = select_busy_enter, .service = select_busy_service,
        .service_predicate = init_service_once, .next_mask = FSM_NEXT(INIT_STATUS_LENGTH) },
    [INIT_STATUS_LENGTH] = INIT_COMMAND_STATE(FSM_NEXT(INIT_STATUS_APP)),
    [INIT_STATUS_APP] = INIT_COMMAND_STATE(FSM_NEXT(INIT_STATUS_DATA)),
    [INIT_STATUS_DATA] = INIT_COMMAND_STATE(FSM_NEXT(INIT_SCR_LENGTH)),
    [INIT_SCR_LENGTH] = INIT_COMMAND_STATE(FSM_NEXT(INIT_SCR_APP)),
    [INIT_SCR_APP] = INIT_COMMAND_STATE(FSM_NEXT(INIT_SCR_DATA)),
    [INIT_SCR_DATA] = INIT_COMMAND_STATE(FSM_NEXT(INIT_WIDTH_APP) | FSM_NEXT(INIT_BLOCK_LENGTH)),
    [INIT_WIDTH_APP] = INIT_COMMAND_STATE(FSM_NEXT(INIT_WIDTH)),
    [INIT_WIDTH] = INIT_COMMAND_STATE(FSM_NEXT(INIT_BLOCK_LENGTH)),
    [INIT_BLOCK_LENGTH] = INIT_COMMAND_STATE(FSM_NEXT(INIT_SPEED_SELECT)),
    [INIT_SPEED_SELECT] = INIT_SERVICE_STATE(speed_select_service, FSM_NEXT(INIT_SPEED_CHECK) | FSM_NEXT(INIT_BUS)),
    [INIT_SPEED_CHECK] = INIT_COMMAND_STATE(FSM_NEXT(INIT_BUS) | FSM_NEXT(INIT_SPEED_RETRY) | FSM_NEXT(INIT_SPEED_SWITCH)),
    [INIT_SPEED_RETRY] = { .enter = speed_retry_enter, .next_mask = FSM_NEXT(INIT_SPEED_CHECK) },
    [INIT_SPEED_SWITCH] = INIT_COMMAND_STATE(FSM_NEXT(INIT_SPEED_SETTLE)),
    [INIT_SPEED_SETTLE] = { .enter = speed_settle_enter, .next_mask = FSM_NEXT(INIT_BUS) },
    [INIT_BUS] = INIT_SERVICE_STATE(bus_service, FSM_NEXT(INIT_READY)),
    [INIT_READY] = INIT_COMMAND_STATE(FSM_NEXT(INIT_READY_RETRY)),
    [INIT_READY_RETRY] = { .enter = ready_retry_enter, .next_mask = FSM_NEXT(INIT_READY) },
};
#undef INIT_COMMAND_STATE
#undef INIT_SERVICE_STATE

void SDCard_InitStart(SD_HandleTypeDef *handle, SDCard_Info *info, uint32_t power_on_at) {
    sdcard_init.handle = handle;
    sdcard_init.info = info;
    sdcard_init.init_at = power_on_at;
    sdcard_init.result = SDCARD_INIT_PENDING;
    FSM_Init(&sdcard_init.init_fsm, init_states, INIT_HOST, NULL);
    sdcard_init.init_fsm.transition_at = HAL_GetTick();
}

SDCard_InitResult SDCard_InitService(void) {
    if (sdcard_init.result == SDCARD_INIT_PENDING) {
        sdcard_init.serviced = false;
        FSM_Service(&sdcard_init.init_fsm);
    }
    return sdcard_init.result;
}

void SDCard_InitStop(SD_HandleTypeDef *handle) {
    /* DeInit only resets the host; no command is sent to the removed card.
     * The lifecycle FSM calls this before switching card power off.
     */
    SDMMC1->MASK = 0U;
    SDMMC1->CMD = 0U;
    SDMMC1->DCTRL = 0U;
    HAL_SD_DeInit(handle);
    memset(&sdcard_init.init_fsm, 0, sizeof(sdcard_init.init_fsm));
}
