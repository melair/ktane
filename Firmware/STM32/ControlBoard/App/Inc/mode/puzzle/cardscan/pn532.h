#ifndef CARDSCAN_PN532_H
#define CARDSCAN_PN532_H

#include "i2c/i2c.h"
#include "fsm/fsm.h"

typedef enum {
    PN532_SCAN_NONE = 0,
    PN532_SCAN_VALID,
    PN532_SCAN_INVALID,
} PN532_ScanResult;

typedef struct {
    FSM fsm;
    I2C_Transaction transaction;
    uint8_t tx[16];
    uint8_t rx[32];
    uint8_t uid[7];
    uint8_t last_uid[7];
    FSM_StateId response_state;
    uint8_t command;
    uint8_t card_id;
    PN532_ScanResult result;
    uint32_t deadline_ms;
    uint32_t last_scan_ms;
    bool irq_active;
    bool completed;
    bool last_scan_valid;
} PN532_Data;

void PN532_Init(PN532_Data *reader);
/* Page 4 format: C0 FF EE ID. Writes card_id only for PN532_SCAN_VALID.
 * No target, cooldown suppression and transport errors return PN532_SCAN_NONE. */
PN532_ScanResult PN532_Service(PN532_Data *reader, bool irq_active, uint8_t *card_id);

#endif
