#include "slot.h"

#include "fsm/fsm.h"
#include "i2c/i2c.h"
#include "main.h"

#define SLOT_EEPROM_I2C_ADDRESS   0x50U
#define SLOT_EEPROM_ADDRESS       0x00U
#define SLOT_EEPROM_WRITE_TIME_MS 5U

typedef enum {
    SLOT_FSM_STATE_READ = 0,
    SLOT_FSM_STATE_IDLE,
    SLOT_FSM_STATE_WRITE,
    SLOT_FSM_STATE_RESET,
    SLOT_FSM_STATE_COUNT,
} Slot_FSM_State;

typedef struct {
    FSM fsm;
    uint8_t value;
    uint8_t tx_data[2];
    I2C_Transaction transaction;
} Slot_State;

static Slot_State slot = {
    .value = SLOT_UNKNOWN,
};

static void slot_fsm_read_enter(FSM *fsm);

static void slot_fsm_write_enter(FSM *fsm);

static void slot_fsm_reset_enter(FSM *fsm);

static const FSM_State slot_fsm_states[SLOT_FSM_STATE_COUNT] = {
    [SLOT_FSM_STATE_READ] = {
        .enter = slot_fsm_read_enter,
        .next_mask = FSM_NEXT(SLOT_FSM_STATE_IDLE),
    },
    [SLOT_FSM_STATE_IDLE] = {
        .next_mask = FSM_NEXT(SLOT_FSM_STATE_WRITE),
    },
    [SLOT_FSM_STATE_WRITE] = {
        .enter = slot_fsm_write_enter,
        .next_mask = FSM_NEXT(SLOT_FSM_STATE_IDLE) |
                     FSM_NEXT(SLOT_FSM_STATE_RESET),
    },
    [SLOT_FSM_STATE_RESET] = {
        .enter = slot_fsm_reset_enter,
    },
};

static I2C_Transaction *slot_i2c_complete(I2C_Transaction *transaction) {
    if (transaction->status != I2C_STATUS_SUCCESS) {
        (void) FSM_Transition(&slot.fsm, SLOT_FSM_STATE_IDLE);
        return NULL;
    }

    if (slot.fsm.current_id == SLOT_FSM_STATE_READ) {
        (void) FSM_Transition(&slot.fsm, SLOT_FSM_STATE_IDLE);
    } else if (slot.fsm.current_id == SLOT_FSM_STATE_WRITE) {
        (void) FSM_TransitionIn(&slot.fsm, SLOT_FSM_STATE_RESET,
                                SLOT_EEPROM_WRITE_TIME_MS);
    }

    return NULL;
}

static void slot_fsm_read_enter(FSM *fsm) {
    Slot_State *state = fsm->context;

    state->tx_data[0] = SLOT_EEPROM_ADDRESS;
    state->transaction = (I2C_Transaction) {
        .operation = I2C_OPERATION_WRITE_RESTART_READ,
        .address = SLOT_EEPROM_I2C_ADDRESS,
        .tx_data = state->tx_data,
        .tx_size = 1U,
        .rx_data = &state->value,
        .rx_size = 1U,
        .callback = slot_i2c_complete,
    };
    I2C_Queue(&state->transaction);
}

static void slot_fsm_write_enter(FSM *fsm) {
    Slot_State *state = fsm->context;

    state->transaction = (I2C_Transaction) {
        .operation = I2C_OPERATION_WRITE,
        .address = SLOT_EEPROM_I2C_ADDRESS,
        .tx_data = state->tx_data,
        .tx_size = sizeof(state->tx_data),
        .callback = slot_i2c_complete,
    };
    I2C_Queue(&state->transaction);
}

static void slot_fsm_reset_enter(FSM *fsm) {
    (void) fsm;
    NVIC_SystemReset();
}

void Slot_Init(void) {
    slot.value = SLOT_UNKNOWN;
    slot.tx_data[0] = SLOT_EEPROM_ADDRESS;
    (void) FSM_Init(&slot.fsm, slot_fsm_states, SLOT_FSM_STATE_READ, &slot);
}

void Slot_Service(void) {
    FSM_Service(&slot.fsm);
}

uint8_t Slot_Get(void) {
    return slot.value;
}

bool Slot_Set(const uint8_t new_slot) {
    if (!FSM_Transition(&slot.fsm, SLOT_FSM_STATE_WRITE)) {
        return false;
    }

    slot.tx_data[0] = SLOT_EEPROM_ADDRESS;
    slot.tx_data[1] = new_slot;

    return true;
}
