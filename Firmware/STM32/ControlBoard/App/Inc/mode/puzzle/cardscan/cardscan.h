#ifndef CARDSCAN_H
#define CARDSCAN_H

#include "mode_fsm.h"
#include "mode/puzzle/cardscan/pn532.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t startup_step;
    uint32_t startup_next_step_ms;
    PN532_Data nfc;
    uint8_t scanned_id;
    bool scanned_updated;
    bool red_startup_on;
    bool red_flash_active;
    uint32_t red_flash_until_ms;
    bool yellow_startup_on;
    bool yellow_flash_active;
    uint32_t yellow_flash_until_ms;
} CardScan_Data;

extern Mode_Definition cardscan_mode;

#ifdef __cplusplus
}
#endif

#endif //CARDSCAN_H
