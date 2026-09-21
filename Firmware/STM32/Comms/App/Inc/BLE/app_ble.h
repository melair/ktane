/**
  ******************************************************************************
  * @file    app_ble.h
  * @author  MCD Application Team
  * @brief   Header for ble application
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef APP_BLE_H
#define APP_BLE_H

#ifdef __cplusplus
extern "C" {

#endif

/* Includes ------------------------------------------------------------------*/
#include "ble_events.h"
#include "ble_status.h"
/* Private includes ----------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/

typedef enum {
  APP_BLE_ADV_STOPPED,
  APP_BLE_ADV_FAST,
  APP_BLE_ADV_LP,
} APP_BLE_AdvertisingStatus_t;

typedef enum {
  APP_BLE_CONNECTED,
  APP_BLE_DISCONNECTING,
} APP_BLE_ConnectionStatus_t;

#define APP_BLE_MAX_CONNECTIONS (4U)

typedef struct {
  uint16_t handle;
  uint8_t mac[6]; /* Bluetooth stack byte order, least-significant byte first. */
  uint8_t address_type;
  APP_BLE_ConnectionStatus_t status;
} APP_BLE_Connection_t;

typedef enum {
  PROC_GAP_PERIPH_ADVERTISE_START_LP,
  PROC_GAP_PERIPH_ADVERTISE_START_FAST,
  PROC_GAP_PERIPH_ADVERTISE_STOP,
  PROC_GAP_PERIPH_ADVERTISE_DATA_UPDATE,

  PROC_GAP_PERIPH_SET_BROADCAST_MODE,

} ProcGapPeripheralId_t;

typedef enum {
  PROC_GAP_CENTRAL_SCAN_START,
  PROC_GAP_CENTRAL_SCAN_TERMINATE,

} ProcGapCentralId_t;

/* Exported constants --------------------------------------------------------*/

/* External variables --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/
#define SCAN_WIN_MS(x) ((uint16_t)((x)/0.625f))
#define SCAN_INT_MS(x) ((uint16_t)((x)/0.625f))
#define CONN_INT_MS(x) ((uint16_t)((x)/1.25f))
#define CONN_SUP_TIMEOUT_MS(x) ((uint16_t)((x)/10.0f))
#define CONN_CE_LENGTH_MS(x) ((uint16_t)((x)/0.625f))

/* Exported functions ---------------------------------------------*/
void ModulesInit(void);

void BLE_Init(void);

void APP_BLE_Init(void);

/* Call these APIs from the BLE application context, serialized with events.
 * Queries copy records; enumeration indices may change on disconnection.
 * NULL outputs/invalid indices return INVALID_PARAMS; unknown handles return
 * UNKNOWN_CONNECTION_ID. Disconnected peers have no record.
 */
uint8_t APP_BLE_GetConnectionCount(void);

tBleStatus APP_BLE_GetConnection(uint16_t handle, APP_BLE_Connection_t *out);

tBleStatus APP_BLE_GetConnectionByIndex(uint8_t index, APP_BLE_Connection_t *out);

APP_BLE_AdvertisingStatus_t APP_BLE_GetAdvertisingStatus(void);

/* Success means the stack accepted the request, not asynchronous completion.
 * Unknown handles return UNKNOWN_CONNECTION_ID; disconnecting peers return BUSY.
 */
tBleStatus APP_BLE_Disconnect(uint16_t handle);

tBleStatus APP_BLE_TogglePhy(uint16_t handle);

tBleStatus APP_BLE_ExchangeMtu(uint16_t handle);

tBleStatus APP_BLE_RequestConnectionParameterUpdate(uint16_t handle);

/**
 * Enable or disable pairing with previously unknown BLE clients.
 *
 * A non-zero value enables pairing; zero restricts scan and connection requests
 * to peers in the controller Filter Accept List.
 */
void APP_BLE_SetPairingMode(uint8_t enabled);

/**
 * Register or update one of four device-specific pairing PINs in RAM.
 * MAC uses stack byte order: AA:BB:CC:DD:EE:FF is {FF, EE, DD, CC, BB, AA}.
 * PIN range is 0..999999 (displayed with six digits, including leading zeros).
 * Returns BLE_STATUS_INVALID_PARAMS for NULL/all-FF MAC or an invalid PIN,
 * or BLE_STATUS_INSUFFICIENT_RESOURCES if all four entries are occupied.
 * Entries are cleared on each pairing-mode transition, but not same-mode calls.
 * Enable pairing before populating the table. Call from the BLE application
 * context, serialized with BLE event handling and APP_BLE_SetPairingMode.
 */
tBleStatus APP_BLE_SetPairingPin(const uint8_t mac[6], uint32_t pin);

/**
 * Erase all bonded-peer records and clear the controller accept/resolving lists.
 *
 * Returns BLE_STATUS_BUSY if any BLE link remains connected.
 */
tBleStatus APP_BLE_ClearPairingInformation(void);

void APP_BLE_Procedure_Gap_Peripheral(ProcGapPeripheralId_t ProcGapPeripheralId);

#ifdef __cplusplus
}
#endif

#endif /*APP_BLE_H */
