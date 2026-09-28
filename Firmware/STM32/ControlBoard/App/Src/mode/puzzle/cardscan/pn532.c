#include "mode/puzzle/cardscan/pn532.h"
#include "stm32h5xx_hal.h"
#include <string.h>

#define PN532_ADDRESS 0x24U
#define COMMAND_TIMEOUT_MS 1500U
#define SCAN_INTERVAL_MS 250U
#define SAME_CARD_COOLDOWN_MS 2500U

typedef enum {
    PN532_STATE_POWER_ON,
    PN532_STATE_SAM,
    PN532_STATE_RETRIES,
    PN532_STATE_DETECT,
    PN532_STATE_READ_CARD,
    PN532_STATE_WRITE_WAIT,
    PN532_STATE_ACK_WAIT,
    PN532_STATE_ACK_READ,
    PN532_STATE_RESPONSE_WAIT,
    PN532_STATE_RESPONSE_READ,
    PN532_STATE_SAM_DONE,
    PN532_STATE_RETRIES_DONE,
    PN532_STATE_DETECT_DONE,
    PN532_STATE_READ_CARD_DONE,
    PN532_STATE_COOLDOWN,
    PN532_STATE_RECOVERY,
    PN532_STATE_CANCEL,
} PN532_State;

static const uint8_t ack[] = {0x00, 0x00, 0xff, 0x00, 0xff, 0x00};

static I2C_Transaction *transfer_complete(I2C_Transaction *transaction) {
    PN532_Data *reader = transaction->callback_data;
    reader->completed = true;
    return NULL;
}

static void queue(PN532_Data *reader, bool read, uint16_t size) {
    reader->transaction.operation = read ? I2C_OPERATION_READ : I2C_OPERATION_WRITE;
    reader->transaction.tx_size = read ? 0U : size;
    reader->transaction.rx_size = read ? size : 0U;
    reader->completed = false;
    I2C_Queue(&reader->transaction);
}

static void pn532_fsm_power_on_enter(FSM *fsm) {
    FSM_TransitionIn(fsm, PN532_STATE_SAM, 500U);
}

static void pn532_fsm_recovery_enter(FSM *fsm) {
    FSM_TransitionIn(fsm, PN532_STATE_CANCEL, 500U);
}

static void pn532_fsm_cooldown_enter(FSM *fsm) {
    FSM_TransitionIn(fsm, PN532_STATE_DETECT, SCAN_INTERVAL_MS);
}

static void send_command(FSM *fsm, uint8_t command, const uint8_t *payload,
                         uint8_t size, PN532_State response_state) {
    PN532_Data *reader = fsm->context;
    reader->command = command;
    reader->response_state = response_state;
    uint8_t *frame = reader->tx;
    frame[0] = 0x00;
    frame[1] = 0x00;
    frame[2] = 0xff;
    frame[3] = size + 2U;
    frame[4] = (uint8_t) -frame[3];
    frame[5] = 0xd4;
    frame[6] = command;
    memcpy(frame + 7, payload, size);
    uint8_t sum = (uint8_t) (0xd4U + command);
    for (uint8_t i = 0; i < size; ++i) {
        sum += payload[i];
    }
    frame[7U + size] = (uint8_t) -sum;
    frame[8U + size] = 0x00;
    reader->deadline_ms = HAL_GetTick() + COMMAND_TIMEOUT_MS;
    queue(reader, false, size + 9U);
    FSM_Transition(fsm, PN532_STATE_WRITE_WAIT);
}

static void pn532_fsm_sam_enter(FSM *fsm) {
    const uint8_t payload[] = {0x01, 0x00, 0x01}; /* Normal mode, IRQ enabled. */
    send_command(fsm, 0x14, payload, sizeof(payload), PN532_STATE_SAM_DONE);
}

static void pn532_fsm_retries_enter(FSM *fsm) {
    const uint8_t payload[] = {0x05, 0xff, 0x01, 0x00}; /* One passive activation attempt. */
    send_command(fsm, 0x32, payload, sizeof(payload), PN532_STATE_RETRIES_DONE);
}

static void pn532_fsm_detect_enter(FSM *fsm) {
    const uint8_t payload[] = {0x01, 0x00}; /* One target, 106 kbps Type A. */
    send_command(fsm, 0x4a, payload, sizeof(payload), PN532_STATE_DETECT_DONE);
}

static void pn532_fsm_read_card_enter(FSM *fsm) {
    /* Target 1, READ pages 4-7. Page 4 contains C0 FF EE and the card ID. */
    const uint8_t payload[] = {0x01, 0x30, 0x04};
    send_command(fsm, 0x40, payload, sizeof(payload), PN532_STATE_READ_CARD_DONE);
}

static void pn532_fsm_write_wait_service(FSM *fsm) {
    const PN532_Data *reader = fsm->context;
    if (!reader->completed) {
        return;
    }
    FSM_Transition(fsm, reader->transaction.status == I2C_STATUS_SUCCESS
                       ? PN532_STATE_ACK_WAIT : PN532_STATE_RECOVERY);
}

static void wait_for_irq(FSM *fsm, PN532_State ready_state) {
    const PN532_Data *reader = fsm->context;
    if ((int32_t) (HAL_GetTick() - reader->deadline_ms) >= 0) {
        FSM_Transition(fsm, PN532_STATE_RECOVERY);
    } else if (reader->irq_active) {
        FSM_Transition(fsm, ready_state);
    }
}

static void pn532_fsm_ack_wait_service(FSM *fsm) {
    wait_for_irq(fsm, PN532_STATE_ACK_READ);
}

static void pn532_fsm_response_wait_service(FSM *fsm) {
    wait_for_irq(fsm, PN532_STATE_RESPONSE_READ);
}

static void pn532_fsm_ack_read_enter(FSM *fsm) {
    queue(fsm->context, true, 7U);
}

static void pn532_fsm_ack_read_service(FSM *fsm) {
    const PN532_Data *reader = fsm->context;
    if (!reader->completed) {
        return;
    }
    const bool valid = reader->transaction.status == I2C_STATUS_SUCCESS &&
                       (reader->rx[0] & 1U) != 0U &&
                       memcmp(reader->rx + 1, ack, sizeof(ack)) == 0;
    FSM_Transition(fsm, valid ? PN532_STATE_RESPONSE_WAIT : PN532_STATE_RECOVERY);
}

static void pn532_fsm_response_read_enter(FSM *fsm) {
    PN532_Data *reader = fsm->context;
    /* Read ready byte and full frame together: STOP discards unread bytes.
     * Ignore padding after short responses using the frame's LEN. */
    queue(reader, true, sizeof(reader->rx));
}

static bool response_valid(const PN532_Data *reader) {
    const uint8_t *f = reader->rx + 1;
    const uint8_t length = f[3];
    if (reader->transaction.status != I2C_STATUS_SUCCESS || (reader->rx[0] & 1U) == 0U ||
        f[0] != 0 || f[1] != 0 || f[2] != 0xff || length < 2U ||
        (uint8_t) (length + f[4]) != 0U || (uint16_t) length + 8U > sizeof(reader->rx)) {
        return false;
    }
    uint8_t sum = 0;
    for (uint16_t i = 5; i <= 5U + length; ++i) {
        sum += f[i];
    }
    return sum == 0U && f[6U + length] == 0U && f[5] == 0xd5 &&
           f[6] == (uint8_t) (reader->command + 1U);
}

static void pn532_fsm_response_read_service(FSM *fsm) {
    const PN532_Data *reader = fsm->context;
    if (!reader->completed) {
        return;
    }
    FSM_Transition(fsm, response_valid(reader) ? reader->response_state : PN532_STATE_RECOVERY);
}

static void configuration_done(FSM *fsm, PN532_State next_state) {
    const PN532_Data *reader = fsm->context;
    if (reader->rx[4] != 2U) {
        FSM_Transition(fsm, PN532_STATE_RECOVERY);
    } else {
        FSM_Transition(fsm, next_state);
    }
}

static void pn532_fsm_sam_done_enter(FSM *fsm) {
    configuration_done(fsm, PN532_STATE_RETRIES);
}

static void pn532_fsm_retries_done_enter(FSM *fsm) {
    configuration_done(fsm, PN532_STATE_DETECT);
}

static void pn532_fsm_detect_done_enter(FSM *fsm) {
    PN532_Data *reader = fsm->context;
    const uint8_t *data = reader->rx + 8;
    const uint8_t length = reader->rx[4] - 2U;
    if (length != 13U || data[0] != 1U || data[1] != 1U ||
        data[2] != 0x00 || data[3] != 0x44 || data[4] != 0U || data[5] != 7U) {
        reader->result = length > 0U && data[0] != 0U ? PN532_SCAN_INVALID : PN532_SCAN_NONE;
        FSM_Transition(fsm, PN532_STATE_COOLDOWN);
        return;
    }
    memcpy(reader->uid, data + 6, sizeof(reader->uid));
    if (reader->last_scan_valid && memcmp(reader->uid, reader->last_uid, 7) == 0 &&
        (uint32_t) (HAL_GetTick() - reader->last_scan_ms) < SAME_CARD_COOLDOWN_MS) {
        FSM_Transition(fsm, PN532_STATE_COOLDOWN);
    } else {
        FSM_Transition(fsm, PN532_STATE_READ_CARD);
    }
}

static void pn532_fsm_read_card_done_enter(FSM *fsm) {
    PN532_Data *reader = fsm->context;
    const uint8_t *data = reader->rx + 8;
    const uint8_t length = reader->rx[4] - 2U;
    reader->result = PN532_SCAN_INVALID;
    /* InDataExchange status, then pages 4-7. Only page 4 is used. */
    if (length == 17U && data[0] == 0U &&
        data[1] == 0xc0 && data[2] == 0xff && data[3] == 0xee) {
        reader->card_id = data[4];
        memcpy(reader->last_uid, reader->uid, 7);
        reader->last_scan_ms = HAL_GetTick();
        reader->last_scan_valid = true;
        reader->result = PN532_SCAN_VALID;
    }
    FSM_Transition(fsm, PN532_STATE_COOLDOWN);
}

static void pn532_fsm_cancel_enter(FSM *fsm) {
    PN532_Data *reader = fsm->context;
    /* Host ACK aborts a pending command before reconfiguration. */
    memcpy(reader->tx, ack, sizeof(ack));
    queue(reader, false, sizeof(ack));
}

static void pn532_fsm_cancel_service(FSM *fsm) {
    const PN532_Data *reader = fsm->context;
    if (!reader->completed) {
        return;
    }
    /* Even a failed cancellation retries initialization after the power-on delay. */
    FSM_Transition(fsm, PN532_STATE_POWER_ON);
}

static const FSM_State pn532_states[] = {
    [PN532_STATE_POWER_ON] = {
        .enter = pn532_fsm_power_on_enter,
        .next_mask = FSM_NEXT(PN532_STATE_SAM),
    },
    [PN532_STATE_SAM] = {
        .enter = pn532_fsm_sam_enter,
        .next_mask = FSM_NEXT(PN532_STATE_WRITE_WAIT),
    },
    [PN532_STATE_RETRIES] = {
        .enter = pn532_fsm_retries_enter,
        .next_mask = FSM_NEXT(PN532_STATE_WRITE_WAIT),
    },
    [PN532_STATE_DETECT] = {
        .enter = pn532_fsm_detect_enter,
        .next_mask = FSM_NEXT(PN532_STATE_WRITE_WAIT),
    },
    [PN532_STATE_READ_CARD] = {
        .enter = pn532_fsm_read_card_enter,
        .next_mask = FSM_NEXT(PN532_STATE_WRITE_WAIT),
    },
    [PN532_STATE_WRITE_WAIT] = {
        .service = pn532_fsm_write_wait_service,
        .next_mask = FSM_NEXT(PN532_STATE_ACK_WAIT) | FSM_NEXT(PN532_STATE_RECOVERY),
    },
    [PN532_STATE_ACK_WAIT] = {
        .service = pn532_fsm_ack_wait_service,
        .next_mask = FSM_NEXT(PN532_STATE_ACK_READ) | FSM_NEXT(PN532_STATE_RECOVERY),
    },
    [PN532_STATE_ACK_READ] = {
        .enter = pn532_fsm_ack_read_enter,
        .service = pn532_fsm_ack_read_service,
        .next_mask = FSM_NEXT(PN532_STATE_RESPONSE_WAIT) | FSM_NEXT(PN532_STATE_RECOVERY),
    },
    [PN532_STATE_RESPONSE_WAIT] = {
        .service = pn532_fsm_response_wait_service,
        .next_mask = FSM_NEXT(PN532_STATE_RESPONSE_READ) | FSM_NEXT(PN532_STATE_RECOVERY),
    },
    [PN532_STATE_RESPONSE_READ] = {
        .enter = pn532_fsm_response_read_enter,
        .service = pn532_fsm_response_read_service,
        .next_mask = FSM_NEXT(PN532_STATE_SAM_DONE) | FSM_NEXT(PN532_STATE_RETRIES_DONE) |
                     FSM_NEXT(PN532_STATE_DETECT_DONE) | FSM_NEXT(PN532_STATE_READ_CARD_DONE) |
                     FSM_NEXT(PN532_STATE_RECOVERY),
    },
    [PN532_STATE_SAM_DONE] = {
        .enter = pn532_fsm_sam_done_enter,
        .next_mask = FSM_NEXT(PN532_STATE_RETRIES) | FSM_NEXT(PN532_STATE_RECOVERY),
    },
    [PN532_STATE_RETRIES_DONE] = {
        .enter = pn532_fsm_retries_done_enter,
        .next_mask = FSM_NEXT(PN532_STATE_DETECT) | FSM_NEXT(PN532_STATE_RECOVERY),
    },
    [PN532_STATE_DETECT_DONE] = {
        .enter = pn532_fsm_detect_done_enter,
        .next_mask = FSM_NEXT(PN532_STATE_READ_CARD) | FSM_NEXT(PN532_STATE_COOLDOWN),
    },
    [PN532_STATE_READ_CARD_DONE] = {
        .enter = pn532_fsm_read_card_done_enter,
        .next_mask = FSM_NEXT(PN532_STATE_COOLDOWN),
    },
    [PN532_STATE_COOLDOWN] = {
        .enter = pn532_fsm_cooldown_enter,
        .next_mask = FSM_NEXT(PN532_STATE_DETECT),
    },
    [PN532_STATE_RECOVERY] = {
        .enter = pn532_fsm_recovery_enter,
        .next_mask = FSM_NEXT(PN532_STATE_CANCEL),
    },
    [PN532_STATE_CANCEL] = {
        .enter = pn532_fsm_cancel_enter,
        .service = pn532_fsm_cancel_service,
        .next_mask = FSM_NEXT(PN532_STATE_POWER_ON),
    },
};

void PN532_Init(PN532_Data *reader) {
    memset(reader, 0, sizeof(*reader));
    reader->transaction = (I2C_Transaction) {
        .address = PN532_ADDRESS,
        .tx_data = reader->tx,
        .rx_data = reader->rx,
        .callback = transfer_complete,
        .callback_data = reader,
    };
    FSM_Init(&reader->fsm, pn532_states, PN532_STATE_POWER_ON, reader);
}

PN532_ScanResult PN532_Service(PN532_Data *reader, bool irq_active, uint8_t *card_id) {
    reader->irq_active = irq_active;
    reader->result = PN532_SCAN_NONE;
    FSM_Service(&reader->fsm);
    if (reader->result == PN532_SCAN_VALID) {
        *card_id = reader->card_id;
    }
    return reader->result;
}
