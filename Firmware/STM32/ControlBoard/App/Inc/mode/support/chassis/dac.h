#ifndef DAC_H
#define DAC_H

#include <stdbool.h>
#include <stdint.h>
#include "fsm.h"
#include "i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    FSM fsm;
    I2C_Transaction transaction;
    FSM_StateId transaction_next_state;
    uint8_t tx_data[21];
    uint16_t volume_write;
    int8_t volume;
    bool mute_override;
    bool ready;
} DAC_Data;

void DAC_Init(void);

void DAC_Service(void);

bool DAC_Ready(void);

/* Volume ranges from -50 dB to 0 dB. */
void DAC_Volume(int8_t volume_db);

int8_t DAC_GetVolume(void);

void DAC_Mute(bool muted);

bool DAC_IsMuted(void);

#ifdef __cplusplus
}
#endif

#endif //DAC_H
