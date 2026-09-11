#ifndef SLOT_H
#define SLOT_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SLOT_UNKNOWN 0xffU

/**
 * Queue the boot-time read of the slot position from the external EEPROM.
 *
 * I2C_Init() must be called first. Until the transaction completes,
 * Slot_Get() returns SLOT_UNKNOWN.
 */
void Slot_Init(void);

/**
 * Service queued slot transactions and the delayed reset after a write.
 */
void Slot_Service(void);

/**
 * Get the slot position most recently read from the EEPROM.
 */
uint8_t Slot_Get(void);

/**
 * Store a new slot position in the EEPROM.
 *
 * @return true when the write transaction was queued. The MCU resets after
 *         the EEPROM acknowledges the write; false when another slot
 *         transaction is already in progress.
 */
bool Slot_Set(uint8_t slot);

#ifdef __cplusplus
}
#endif

#endif // SLOT_H
