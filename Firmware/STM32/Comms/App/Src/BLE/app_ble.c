/**
  ******************************************************************************
  * @file    app_ble.c
  * @author  GPM WBL Application Team
  * @brief   BLE Application
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

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "main.h"
#include "stm32wb0x.h"
#include "ble.h"
#include "gatt_profile.h"
#include "gap_profile.h"
#include "app_ble.h"
#include "stm32wb0x_hal_radio_timer.h"
#include "nvm_db.h"
#include "blenvm.h"
#include "pka_manager.h"
#include "stm32_seq.h"
#include "ktane_gatt.h"
/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

#define MAX_PERIPHERAL_CONNECTIONS APP_BLE_MAX_CONNECTIONS
#define KTANE_AD_TYPE_APPEARANCE   (0x19U)
#define KTANE_ADV_FLAGS_INDEX     (2U)

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

NO_INIT(uint32_t dyn_alloc_a[BLE_DYN_ALLOC_SIZE>>2]);
static APP_BLE_Connection_t peripheral_connections[MAX_PERIPHERAL_CONNECTIONS];

#define PAIRING_PIN_CAPACITY (4U)
#define EMPTY_PAIRING_MAC {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}
static const uint8_t empty_pairing_mac[6] = EMPTY_PAIRING_MAC;

static struct {
  uint8_t mac[6];
  uint32_t pin;
} pairing_pins[PAIRING_PIN_CAPACITY] = {
  {EMPTY_PAIRING_MAC, 0U}, {EMPTY_PAIRING_MAC, 0U},
  {EMPTY_PAIRING_MAC, 0U}, {EMPTY_PAIRING_MAC, 0U},
};

static uint8_t peripheral_connection_count;
/* Pairing is locked by default; the application explicitly opens it when needed. */
static uint8_t pairing_mode_enabled = 0U;
/* Advertising may remain active while other peers are connected. */
static APP_BLE_AdvertisingStatus_t advertising_status = APP_BLE_ADV_STOPPED;

static const char a_GapDeviceName[] = {'K', 'T', 'A', 'N', 'E', ' ', 'B', 'o', 'm', 'b'};

/**
 * Advertising Data
 */
uint8_t a_AdvData[] =
{
  2, AD_TYPE_FLAGS, FLAG_BIT_LE_GENERAL_DISCOVERABLE_MODE | FLAG_BIT_BR_EDR_NOT_SUPPORTED,
  3, KTANE_AD_TYPE_APPEARANCE, (uint8_t) CFG_GAP_APPEARANCE,
  (uint8_t) (CFG_GAP_APPEARANCE >> 8),
  17, AD_TYPE_128_BIT_SERV_UUID_CMPLT_LIST,
  KTANE_DISCOVERY_UUID_128,
};

static uint8_t a_ScanResponseData[] =
{
  11, AD_TYPE_COMPLETE_LOCAL_NAME, 'K', 'T', 'A', 'N', 'E', ' ', 'B', 'o', 'm', 'b',
};

/* Private function prototypes -----------------------------------------------*/
static void connection_complete_event(uint8_t Status,
                                      uint16_t Connection_Handle,
                                      uint8_t Role,
                                      uint8_t Peer_Address_Type,
                                      uint8_t Peer_Address[6],
                                      uint16_t Connection_Interval,
                                      uint16_t Peripheral_Latency,
                                      uint16_t Supervision_Timeout);

static uint8_t track_peripheral_connection(uint16_t connection_handle, const uint8_t mac[6], uint8_t address_type);

static uint8_t untrack_peripheral_connection(uint16_t connection_handle);

static APP_BLE_Connection_t *find_connection(uint16_t handle);

static tBleStatus select_pairing_pin(uint16_t connection_handle, uint32_t *pin);

static tBleStatus refresh_bonded_device_lists(void);

/* External variables --------------------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/* Functions Definition ------------------------------------------------------*/
void ModulesInit(void) {
  BLENVM_Init();
  if (PKAMGR_Init() == PKAMGR_ERROR) {
    Error_Handler();
  }
}

void BLE_Init(void) {
  uint8_t role;
  uint8_t privacy_type = 0;
  uint16_t gatt_service_changed_handle;
  uint16_t gap_dev_name_char_handle;
  uint16_t gap_appearance_char_handle;
  uint16_t gap_periph_pref_conn_param_char_handle;
  uint8_t bd_address[6] = {0};
  uint8_t bd_address_len = 6;
  uint16_t appearance = CFG_GAP_APPEARANCE;

  BLE_STACK_InitTypeDef BLE_STACK_InitParams = {
    .BLEStartRamAddress = (uint8_t *) dyn_alloc_a,
    .TotalBufferSize = BLE_DYN_ALLOC_SIZE,
    .NumAttrRecords = CFG_BLE_NUM_GATT_ATTRIBUTES,
    .MaxNumOfClientProcs = CFG_BLE_NUM_OF_CONCURRENT_GATT_CLIENT_PROC,
    .NumOfRadioTasks = CFG_BLE_NUM_RADIO_TASKS,
    .NumOfEATTChannels = CFG_BLE_NUM_EATT_CHANNELS,
    .NumBlockCount = CFG_BLE_MBLOCKS_COUNT,
    .ATT_MTU = CFG_BLE_ATT_MTU_MAX,
    .MaxConnEventLength = CFG_BLE_CONN_EVENT_LENGTH_MAX,
    .SleepClockAccuracy = CFG_BLE_SLEEP_CLOCK_ACCURACY,
    .NumOfAdvDataSet = CFG_BLE_NUM_ADV_SETS,
    .NumOfSubeventsPAwR = CFG_BLE_NUM_PAWR_SUBEVENTS,
    .MaxPAwRSubeventDataCount = CFG_BLE_PAWR_SUBEVENT_DATA_COUNT_MAX,
    .NumOfAuxScanSlots = CFG_BLE_NUM_AUX_SCAN_SLOTS,
    .FilterAcceptListSizeLog2 = CFG_BLE_FILTER_ACCEPT_LIST_SIZE_LOG2,
    .L2CAP_MPS = CFG_BLE_COC_MPS_MAX,
    .L2CAP_NumChannels = CFG_BLE_COC_NBR_MAX,
    .NumOfSyncSlots = CFG_BLE_NUM_SYNC_SLOTS,
    .CTE_MaxNumAntennaIDs = CFG_BLE_NUM_CTE_ANTENNA_IDS_MAX,
    .CTE_MaxNumIQSamples = CFG_BLE_NUM_CTE_IQ_SAMPLES_MAX,
    .NumOfSyncBIG = CFG_BLE_NUM_SYNC_BIG_MAX,
    .NumOfBrcBIG = CFG_BLE_NUM_BRC_BIG_MAX,
    .NumOfSyncBIS = CFG_BLE_NUM_SYNC_BIS_MAX,
    .NumOfBrcBIS = CFG_BLE_NUM_BRC_BIS_MAX,
    .NumOfCIG = CFG_BLE_NUM_CIG_MAX,
    .NumOfCIS = CFG_BLE_NUM_CIS_MAX,
    .ExtraLLProcedureContexts = CFG_BLE_EXTRA_LL_PROCEDURE_CONTEXTS,
    .isr0_fifo_size = CFG_BLE_ISR0_FIFO_SIZE,
    .isr1_fifo_size = CFG_BLE_ISR1_FIFO_SIZE,
    .user_fifo_size = CFG_BLE_USER_FIFO_SIZE
  };

  /* Bluetooth LE stack init */
  tBleStatus ret = BLE_STACK_Init(&BLE_STACK_InitParams);
  if (ret != BLE_STATUS_SUCCESS) {
    APP_DBG_MSG("Error in BLE_STACK_Init() 0x%02x\r\n", ret);
    Error_Handler();
  }

#if (CFG_BD_ADDRESS_TYPE == HCI_ADDR_PUBLIC)

  bd_address[0] = (uint8_t) ((CFG_PUBLIC_BD_ADDRESS & 0x0000000000FF));
  bd_address[1] = (uint8_t) ((CFG_PUBLIC_BD_ADDRESS & 0x00000000FF00) >> 8);
  bd_address[2] = (uint8_t) ((CFG_PUBLIC_BD_ADDRESS & 0x000000FF0000) >> 16);
  bd_address[3] = (uint8_t) ((CFG_PUBLIC_BD_ADDRESS & 0x0000FF000000) >> 24);
  bd_address[4] = (uint8_t) ((CFG_PUBLIC_BD_ADDRESS & 0x00FF00000000) >> 32);
  bd_address[5] = (uint8_t) ((CFG_PUBLIC_BD_ADDRESS & 0xFF0000000000) >> 40);
  (void) bd_address_len;

  ret = aci_hal_write_config_data(CONFIG_DATA_PUBADDR_OFFSET, CONFIG_DATA_PUBADDR_LEN, bd_address);
  if (ret != BLE_STATUS_SUCCESS) {
    APP_DBG_MSG("  Fail   : aci_hal_write_config_data command - CONFIG_DATA_PUBADDR_OFFSET, result: 0x%02X\n", ret);
  } else {
    APP_DBG_MSG("  Success: aci_hal_write_config_data command - CONFIG_DATA_PUBADDR_OFFSET\n");
  }
#endif

  /**
   * Set TX Power.
   */
  ret = aci_hal_set_tx_power_level(CFG_TX_POWER_MODE, CFG_TX_POWER);
  if (ret != BLE_STATUS_SUCCESS) {
    APP_DBG_MSG("  Fail   : aci_hal_set_tx_power_level command, result: 0x%02X\n", ret);
  } else {
    APP_DBG_MSG("  Success: aci_hal_set_tx_power_level command\n");
  }

  /**
   * Initialize GATT interface
   */
  ret = aci_gatt_srv_profile_init(GATT_INIT_SERVICE_CHANGED_BIT, &gatt_service_changed_handle);
  if (ret != BLE_STATUS_SUCCESS) {
    APP_DBG_MSG("  Fail   : aci_gatt_srv_profile_init command, result: 0x%02X\n", ret);
  } else {
    APP_DBG_MSG("  Success: aci_gatt_srv_profile_init command\n");
  }

  /**
   * Initialize GAP interface
   */
  role = 0U;
  role |= GAP_PERIPHERAL_ROLE;

#if CFG_BLE_PRIVACY_ENABLED
  privacy_type = 0x02;
#endif

  ret = aci_gap_init(privacy_type, CFG_BD_ADDRESS_TYPE);
  if (ret != BLE_STATUS_SUCCESS) {
    APP_DBG_MSG("  Fail   : aci_gap_init command, result: 0x%02X\n", ret);
  } else {
    APP_DBG_MSG("  Success: aci_gap_init command\n");
  }

  ret = aci_gap_profile_init(role, privacy_type,
                             &gap_dev_name_char_handle,
                             &gap_appearance_char_handle,
                             &gap_periph_pref_conn_param_char_handle);

#if (CFG_BD_ADDRESS_TYPE == HCI_ADDR_STATIC_RANDOM_ADDR)
  ret = aci_hal_read_config_data(CONFIG_DATA_STORED_STATIC_RANDOM_ADDRESS,
                                 &bd_address_len, bd_address);
  APP_DBG_MSG("  Static Random Bluetooth Address: %02x:%02x:%02x:%02x:%02x:%02x\n", bd_address[5], bd_address[4],
              bd_address[3], bd_address[2], bd_address[1], bd_address[0]);
#elif (CFG_BD_ADDRESS_TYPE == HCI_ADDR_PUBLIC)
  APP_DBG_MSG("  Public Bluetooth Address: %02x:%02x:%02x:%02x:%02x:%02x\n", bd_address[5], bd_address[4],
              bd_address[3], bd_address[2], bd_address[1], bd_address[0]);
#else
#error "Invalid CFG_BD_ADDRESS_TYPE"
#endif

  ret = Gap_profile_set_dev_name(0, sizeof(a_GapDeviceName), (uint8_t *) a_GapDeviceName);

  if (ret != BLE_STATUS_SUCCESS) {
    APP_DBG_MSG("  Fail   : Gap_profile_set_dev_name - Device Name, result: 0x%02X\n", ret);
  } else {
    APP_DBG_MSG("  Success: Gap_profile_set_dev_name - Device Name\n");
  }

  ret = Gap_profile_set_appearance(0, sizeof(appearance), (uint8_t *) &appearance);

  if (ret != BLE_STATUS_SUCCESS) {
    APP_DBG_MSG("  Fail   : Gap_profile_set_appearance - Appearance, result: 0x%02X\n", ret);
  } else {
    APP_DBG_MSG("  Success: Gap_profile_set_appearance - Appearance\n");
  }

  /**
   * Initialize IO capability
   */
  ret = aci_gap_set_io_capability(CFG_IO_CAPABILITY);
  if (ret != BLE_STATUS_SUCCESS) {
    APP_DBG_MSG("  Fail   : aci_gap_set_io_capability command, result: 0x%02X\n", ret);
  } else {
    APP_DBG_MSG("  Success: aci_gap_set_io_capability command\n");
  }

  /**
   * Initialize authentication
   */

  ret = aci_gap_set_security_requirements(CFG_BONDING_MODE,
                                          CFG_MITM_PROTECTION,
                                          CFG_SC_SUPPORT,
                                          CFG_KEYPRESS_NOTIFICATION_SUPPORT,
                                          CFG_ENCRYPTION_KEY_SIZE_MIN,
                                          CFG_ENCRYPTION_KEY_SIZE_MAX,
                                          GAP_PAIRING_RESP_NONE);

  if (ret != BLE_STATUS_SUCCESS) {
    APP_DBG_MSG("  Fail   : aci_gap_set_security_requirements command, result: 0x%02X\n", ret);
  } else {
    APP_DBG_MSG("  Success: aci_gap_set_security_requirements command\n");
    APP_DBG_MSG("  Security: IO=%u, bonding=%u, MITM=%u, SC=%u, key size=%u..%u\n",
                CFG_IO_CAPABILITY, CFG_BONDING_MODE, CFG_MITM_PROTECTION,
                CFG_SC_SUPPORT, CFG_ENCRYPTION_KEY_SIZE_MIN, CFG_ENCRYPTION_KEY_SIZE_MAX);
  }

  /* Bonded peers and their IRKs are loaded before each advertising start. */
  APP_DBG_MSG("==>> End BLE_Init function\n");
}

void BLEStack_Process_Schedule(void) {
  /* Keep BLE Stack Process priority low, since there are limited cases
     where stack wants to be rescheduled for busy waiting.  */
  UTIL_SEQ_SetTask(1U << CFG_TASK_BLE_STACK, CFG_SEQ_PRIO_1);
}

static void BLEStack_Process(void) {
  BLE_STACK_Tick();
}

void VTimer_Process(void) {
  HAL_RADIO_TIMER_Tick();
}

void VTimer_Process_Schedule(void) {
  UTIL_SEQ_SetTask(1U << CFG_TASK_VTIMER, CFG_SEQ_PRIO_0);
}

void NVM_Process(void) {
  NVMDB_Tick();
}

void NVM_Process_Schedule(void) {
  UTIL_SEQ_SetTask(1U << CFG_TASK_NVM, CFG_SEQ_PRIO_1);
}

/* Function called from RADIO_TIMER_TXRX_WKUP_IRQHandler() context. */
void HAL_RADIO_TIMER_TxRxWakeUpCallback(void) {
  VTimer_Process_Schedule();
}

/* Function called from RADIO_TIMER_CPU_WKUP_IRQHandler() context. */
void HAL_RADIO_TIMER_CpuWakeUpCallback(void) {
  VTimer_Process_Schedule();
}

/* Function called from RADIO_TXRX_IRQHandler() context. */
void HAL_RADIO_TxRxCallback(uint32_t flags) {
  BLE_STACK_RadioHandler(flags);

  VTimer_Process_Schedule();
  NVM_Process_Schedule();
}

/* Function called from RADIO_RRM_IRQHandler() context. */
void HAL_RADIO_RRMCallback(uint32_t ble_irq_status) {
  BLE_STACK_RRMHandler(ble_irq_status);
}

void BLE_STACK_ProcessRequest(void) {
  BLEStack_Process_Schedule();
}

/* Functions Definition ------------------------------------------------------*/
void APP_BLE_Init(void) {

  UTIL_SEQ_RegTask(1U << CFG_TASK_BLE_STACK, UTIL_SEQ_RFU, BLEStack_Process);
  UTIL_SEQ_RegTask(1U << CFG_TASK_VTIMER, UTIL_SEQ_RFU, VTimer_Process);
  UTIL_SEQ_RegTask(1U << CFG_TASK_NVM, UTIL_SEQ_RFU, NVM_Process);
  ModulesInit();

  memset(peripheral_connections, 0, sizeof(peripheral_connections));
  peripheral_connection_count = 0U;
  advertising_status = APP_BLE_ADV_STOPPED;

  /* Initialization of HCI & GATT & GAP layer */
  BLE_Init();

  /* From here, all initialization are BLE application specific */

  /**
  * Initialize Services and Characteristics.
  */
  APP_DBG_MSG("\n");
  APP_DBG_MSG("Services and Characteristics creation\n");
  KTANE_GATT_Init();
  APP_DBG_MSG("End of Services and Characteristics creation\n");
  APP_DBG_MSG("\n");
}

void BLEEVT_App_Notification(const hci_pckt *hci_pckt) {
  tBleStatus ret = BLE_STATUS_ERROR;
  void *event_data;

  UNUSED(ret);

  if (hci_pckt->type != HCI_EVENT_PKT_TYPE && hci_pckt->type != HCI_EVENT_EXT_PKT_TYPE) {
    /* Not an event */
    return;
  }

  hci_event_pckt *p_event_pckt = (hci_event_pckt *) hci_pckt->data;

  if (hci_pckt->type == HCI_EVENT_PKT_TYPE) {
    event_data = p_event_pckt->data;
  } else {
    /* hci_pckt->type == HCI_EVENT_EXT_PKT_TYPE */
    hci_event_ext_pckt *p_event_pckt = (hci_event_ext_pckt *) hci_pckt->data;
    event_data = p_event_pckt->data;
  }

  switch (p_event_pckt->evt) /* evt field is at same offset in hci_event_pckt and hci_event_ext_pckt */
  {
    case HCI_DISCONNECTION_COMPLETE_EVT_CODE: {
      hci_disconnection_complete_event_rp0 *p_disconnection_complete_event = (hci_disconnection_complete_event_rp0 *)
          p_event_pckt->data;

      APP_DBG_MSG("Disconnect handle 0x%04X, status 0x%02X, reason 0x%02X\n",
                  p_disconnection_complete_event->Connection_Handle,
                  p_disconnection_complete_event->Status,
                  p_disconnection_complete_event->Reason);
      if (p_disconnection_complete_event->Status == BLE_STATUS_SUCCESS) {
        uint8_t removed = untrack_peripheral_connection(p_disconnection_complete_event->Connection_Handle);
        KTANE_GATT_OnDisconnected(p_disconnection_complete_event->Connection_Handle);
        if ((removed != 0U) && (advertising_status == APP_BLE_ADV_STOPPED) &&
            (peripheral_connection_count < MAX_PERIPHERAL_CONNECTIONS)) {
          APP_BLE_Procedure_Gap_Peripheral(PROC_GAP_PERIPH_ADVERTISE_START_FAST);
        }
      } else {
        APP_BLE_Connection_t *connection = find_connection(p_disconnection_complete_event->Connection_Handle);
        if (connection != NULL) {
          connection->status = APP_BLE_CONNECTED;
        }
      }

    }
    break;

    case HCI_LE_META_EVT_CODE: {
      hci_le_meta_event *p_meta_evt = (hci_le_meta_event *) p_event_pckt->data;

      switch (p_meta_evt->subevent) {
        case HCI_LE_CONNECTION_UPDATE_COMPLETE_SUBEVT_CODE: {
          hci_le_connection_update_complete_event_rp0 *p_conn_update_complete = (
            hci_le_connection_update_complete_event_rp0
            *) p_meta_evt->data;
          APP_DBG_MSG(">>== HCI_LE_CONNECTION_UPDATE_COMPLETE_SUBEVT_CODE\n");
          APP_DBG_MSG(
            "     - Connection Interval:   %d.%02d ms\n     - Connection latency:    %d\n     - Supervision Timeout:   %d ms\n",
            INT(p_conn_update_complete->Connection_Interval*1.25),
            FRACTIONAL_2DIGITS(p_conn_update_complete->Connection_Interval*1.25),
            p_conn_update_complete->Peripheral_Latency,
            p_conn_update_complete->Supervision_Timeout*10);
          UNUSED(p_conn_update_complete);

        }
        break;
        case HCI_LE_PHY_UPDATE_COMPLETE_SUBEVT_CODE: {
          hci_le_phy_update_complete_event_rp0 *p_le_phy_update_complete = (hci_le_phy_update_complete_event_rp0 *)
              p_meta_evt
              ->data;
          APP_DBG_MSG("PHY update handle 0x%04X, status 0x%02X, TX %u, RX %u\n",
                      p_le_phy_update_complete->Connection_Handle,
                      p_le_phy_update_complete->Status,
                      p_le_phy_update_complete->TX_PHY,
                      p_le_phy_update_complete->RX_PHY);
          UNUSED(p_le_phy_update_complete);

        }
        break;
        case HCI_LE_ENHANCED_CONNECTION_COMPLETE_SUBEVT_CODE: {
          hci_le_enhanced_connection_complete_event_rp0 *p_enhanced_conn_complete = (
            hci_le_enhanced_connection_complete_event_rp0 *) p_meta_evt->data;

          connection_complete_event(p_enhanced_conn_complete->Status,
                                    p_enhanced_conn_complete->Connection_Handle,
                                    p_enhanced_conn_complete->Role,
                                    p_enhanced_conn_complete->Peer_Address_Type,
                                    p_enhanced_conn_complete->Peer_Address,
                                    p_enhanced_conn_complete->Connection_Interval,
                                    p_enhanced_conn_complete->Peripheral_Latency,
                                    p_enhanced_conn_complete->Supervision_Timeout);
        }
        break;
        case HCI_LE_CONNECTION_COMPLETE_SUBEVT_CODE: {
          hci_le_connection_complete_event_rp0 *p_conn_complete = (hci_le_connection_complete_event_rp0 *) p_meta_evt->
              data;

          connection_complete_event(p_conn_complete->Status,
                                    p_conn_complete->Connection_Handle,
                                    p_conn_complete->Role,
                                    p_conn_complete->Peer_Address_Type,
                                    p_conn_complete->Peer_Address,
                                    p_conn_complete->Connection_Interval,
                                    p_conn_complete->Peripheral_Latency,
                                    p_conn_complete->Supervision_Timeout);
        }
        break;

        default:

          break;
      }
    } /* HCI_LE_META_EVT_CODE */
    break;

    case HCI_VENDOR_EVT_CODE: {
      aci_blecore_event *p_blecore_evt = (aci_blecore_event *) event_data;

      switch (p_blecore_evt->ecode) {

        case ACI_L2CAP_CONNECTION_UPDATE_RESP_VSEVT_CODE: {
          aci_l2cap_connection_update_resp_event_rp0 *p_l2cap_conn_update_resp = (
            aci_l2cap_connection_update_resp_event_rp0
            *) p_blecore_evt->data;
          UNUSED(p_l2cap_conn_update_resp);

        }
        break;
        case ACI_GAP_PROC_COMPLETE_VSEVT_CODE: {
          APP_DBG_MSG(">>== ACI_GAP_PROC_COMPLETE_VSEVT_CODE\n");
          aci_gap_proc_complete_event_rp0 *p_gap_proc_complete = (aci_gap_proc_complete_event_rp0 *) p_blecore_evt->
              data;
          UNUSED(p_gap_proc_complete);

        }
        break;
        case ACI_HAL_END_OF_RADIO_ACTIVITY_VSEVT_CODE:

          break;
        case ACI_GAP_KEYPRESS_NOTIFICATION_VSEVT_CODE: {
          APP_DBG_MSG(">>== ACI_GAP_KEYPRESS_NOTIFICATION_VSEVT_CODE\n");

        }
        break;
        case ACI_GAP_PASSKEY_REQ_VSEVT_CODE: {
          APP_DBG_MSG(">>== ACI_GAP_PASSKEY_REQ_VSEVT_CODE\n");

          aci_gap_passkey_req_event_rp0 *p_passkey_req = (aci_gap_passkey_req_event_rp0 *) p_blecore_evt->data;
          APP_DBG_MSG("     - Connection Handle: 0x%04X\n", p_passkey_req->Connection_Handle);
          uint32_t pin;
          ret = select_pairing_pin(p_passkey_req->Connection_Handle, &pin);
          if (ret != BLE_STATUS_SUCCESS) {
            APP_DBG_MSG("Pairing PIN selection failed: 0x%02X\n", ret);
            ret = APP_BLE_Disconnect(p_passkey_req->Connection_Handle);
            if (ret == BLE_STATUS_UNKNOWN_CONNECTION_ID) {
              /* Reject an unexpected stack link that has no application record. */
              ret = aci_gap_terminate(p_passkey_req->Connection_Handle, BLE_ERROR_TERMINATED_REMOTE_USER);
            }
            if (ret != BLE_STATUS_SUCCESS) {
              APP_DBG_MSG("Pairing connection termination failed: 0x%02X\n", ret);
            }
            break;
          }
          ret = aci_gap_passkey_resp(p_passkey_req->Connection_Handle, pin);
          if (ret != BLE_STATUS_SUCCESS) {
            APP_DBG_MSG("==>> aci_gap_passkey_resp : Fail, reason: 0x%02X\n", ret);
          } else {
            APP_DBG_MSG("==>> aci_gap_passkey_resp : Success\n");
          }

        }
        break;
        case ACI_GAP_PAIRING_COMPLETE_VSEVT_CODE: {
          APP_DBG_MSG(">>== ACI_GAP_PAIRING_COMPLETE_VSEVT_CODE\n");
          aci_gap_pairing_complete_event_rp0 *p_pairing_complete = (aci_gap_pairing_complete_event_rp0 *) p_blecore_evt
              ->data;

          if (p_pairing_complete->Status != 0) {
            APP_DBG_MSG("     - Connection Handle: 0x%04X\n     - Pairing KO\n"
                        "     - Status: 0x%02X\n     - Reason: 0x%02X\n",
                        p_pairing_complete->Connection_Handle,
                        p_pairing_complete->Status, p_pairing_complete->Reason);
            if ((p_pairing_complete->Status == SM_PAIRING_FAILED) &&
                (p_pairing_complete->Reason == AUTH_REQ_CANNOT_BE_MET)) {
              APP_DBG_MSG("     - Authentication requirements cannot be met; check peer/local "
                "IO capabilities, MITM and Secure Connections negotiation\n");
            }
          } else {
            uint8_t security_mode;
            uint8_t security_level;

            APP_DBG_MSG("     - Connection Handle: 0x%04X\n     - Pairing Success\n",
                        p_pairing_complete->Connection_Handle);
            ret = aci_gap_get_security_level(p_pairing_complete->Connection_Handle,
                                             &security_mode, &security_level);
            if (ret == BLE_STATUS_SUCCESS) {
              APP_DBG_MSG("     - Security Mode: %u\n     - Security Level: %u\n",
                          security_mode, security_level);
            } else {
              APP_DBG_MSG("     - aci_gap_get_security_level failed: 0x%02X\n", ret);
            }
          }
          APP_DBG_MSG("\n");

        }
        break;
        case ACI_GATT_SRV_READ_VSEVT_CODE: {
          APP_DBG_MSG(">>== ACI_GATT_SRV_READ_VSEVT_CODE\n");

          aci_gatt_srv_read_event_rp0 *p_read = (aci_gatt_srv_read_event_rp0 *) p_blecore_evt->data;
          uint8_t error_code = BLE_ATT_ERR_INSUFF_AUTHORIZATION;

          APP_DBG_MSG("Handle 0x%04X\n", p_read->Attribute_Handle);

          aci_gatt_srv_resp(p_read->Connection_Handle,
                            p_read->CID,
                            p_read->Attribute_Handle,
                            error_code,
                            0,
                            NULL);

          break;
        }

        default:

          break;
      }
    } /* HCI_VENDOR_EVT_CODE */
    break;

    case HCI_HARDWARE_ERROR_EVT_CODE: {
      hci_hardware_error_event_rp0 *p_hci_hardware_error_event = (hci_hardware_error_event_rp0 *) p_event_pckt->data;

      if (p_hci_hardware_error_event->Hardware_Code <= 0x03) {
        NVIC_SystemReset();
      }
    }
    break;

    default:

      break;
  }
}

static void connection_complete_event(uint8_t Status,
                                      uint16_t Connection_Handle,
                                      uint8_t Role,
                                      uint8_t Peer_Address_Type,
                                      uint8_t Peer_Address[6],
                                      uint16_t Connection_Interval,
                                      uint16_t Peripheral_Latency,
                                      uint16_t Supervision_Timeout) {
  if (Status != 0) {
    APP_DBG_MSG("==>> connection_complete_event Fail, Status: 0x%02X\n", Status);
    return;
  }

  APP_DBG_MSG(">>== hci_le_connection_complete_event - Connection handle: 0x%04X\n", Connection_Handle);
  APP_DBG_MSG("     - Connection established with @:%02x:%02x:%02x:%02x:%02x:%02x\n",
              Peer_Address[5],
              Peer_Address[4],
              Peer_Address[3],
              Peer_Address[2],
              Peer_Address[1],
              Peer_Address[0]);
  APP_DBG_MSG(
    "     - Connection Interval:   %d.%02d ms\n     - Connection latency:    %d\n     - Supervision Timeout: %d ms\n",
    INT(Connection_Interval*1.25),
    FRACTIONAL_2DIGITS(Connection_Interval*1.25),
    Peripheral_Latency,
    Supervision_Timeout * 10);

  if (Role == HCI_ROLE_PERIPHERAL) {
    advertising_status = APP_BLE_ADV_STOPPED; /* Legacy advertising stops on connection. */
  }
  if ((Role != HCI_ROLE_PERIPHERAL) ||
      (track_peripheral_connection(Connection_Handle, Peer_Address, Peer_Address_Type) == 0U)) {
    APP_DBG_MSG("Rejecting untracked connection 0x%04X, role %u\n", Connection_Handle, Role);
    tBleStatus status = aci_gap_terminate(Connection_Handle, BLE_ERROR_TERMINATED_REMOTE_USER);
    if (status != BLE_STATUS_SUCCESS) {
      APP_DBG_MSG("Untracked connection termination failed: 0x%02X\n", status);
    }
    return;
  }

  if (peripheral_connection_count < MAX_PERIPHERAL_CONNECTIONS) {
    APP_BLE_Procedure_Gap_Peripheral(PROC_GAP_PERIPH_ADVERTISE_START_FAST);
  }

} /* end hci_le_connection_complete_event() */

static uint8_t track_peripheral_connection(uint16_t connection_handle, const uint8_t mac[6], uint8_t address_type) {
  for (uint8_t index = 0U; index < peripheral_connection_count; index++) {
    if (peripheral_connections[index].handle == connection_handle) {
      memcpy(peripheral_connections[index].mac, mac, 6U);
      peripheral_connections[index].address_type = address_type;
      return 1U;
    }
  }

  if (peripheral_connection_count >= MAX_PERIPHERAL_CONNECTIONS) {
    return 0U;
  }

  peripheral_connections[peripheral_connection_count].handle = connection_handle;
  memcpy(peripheral_connections[peripheral_connection_count].mac, mac, 6U);
  peripheral_connections[peripheral_connection_count].address_type = address_type;
  peripheral_connections[peripheral_connection_count].status = APP_BLE_CONNECTED;
  peripheral_connection_count++;
  return 1U;
}

static uint8_t untrack_peripheral_connection(uint16_t connection_handle) {
  for (uint8_t index = 0U; index < peripheral_connection_count; index++) {
    if (peripheral_connections[index].handle == connection_handle) {
      peripheral_connection_count--;
      peripheral_connections[index] = peripheral_connections[peripheral_connection_count];
      memset(&peripheral_connections[peripheral_connection_count], 0,
             sizeof(peripheral_connections[0]));
      return 1U;
    }
  }

  return 0U;
}

static APP_BLE_Connection_t *find_connection(uint16_t handle) {
  for (uint8_t index = 0U; index < peripheral_connection_count; index++) {
    if (peripheral_connections[index].handle == handle) {
      return &peripheral_connections[index];
    }
  }
  return NULL;
}

uint8_t APP_BLE_GetConnectionCount(void) {
  return peripheral_connection_count;
}

tBleStatus APP_BLE_GetConnection(uint16_t handle, APP_BLE_Connection_t *out) {
  if (out == NULL) {
    return BLE_STATUS_INVALID_PARAMS;
  }
  const APP_BLE_Connection_t *connection = find_connection(handle);
  if (connection == NULL) {
    return BLE_STATUS_UNKNOWN_CONNECTION_ID;
  }
  *out = *connection;
  return BLE_STATUS_SUCCESS;
}

tBleStatus APP_BLE_GetConnectionByIndex(uint8_t index, APP_BLE_Connection_t *out) {
  if ((out == NULL) || (index >= peripheral_connection_count)) {
    return BLE_STATUS_INVALID_PARAMS;
  }
  *out = peripheral_connections[index];
  return BLE_STATUS_SUCCESS;
}

APP_BLE_AdvertisingStatus_t APP_BLE_GetAdvertisingStatus(void) {
  return advertising_status;
}

tBleStatus APP_BLE_SetPairingPin(const uint8_t mac[6], uint32_t pin) {
  uint8_t available = PAIRING_PIN_CAPACITY;

  if ((mac == NULL) || (pin > 999999U) ||
      (memcmp(mac, empty_pairing_mac, 6U) == 0)) {
    return BLE_STATUS_INVALID_PARAMS;
  }

  for (uint8_t index = 0U; index < PAIRING_PIN_CAPACITY; index++) {
    if (memcmp(pairing_pins[index].mac, mac, 6U) == 0) {
      pairing_pins[index].pin = pin;
      return BLE_STATUS_SUCCESS;
    }
    if ((available == PAIRING_PIN_CAPACITY) &&
        (memcmp(pairing_pins[index].mac, empty_pairing_mac, 6U) == 0)) {
      available = index;
    }
  }

  if (available == PAIRING_PIN_CAPACITY) {
    return BLE_STATUS_INSUFFICIENT_RESOURCES;
  }
  memcpy(pairing_pins[available].mac, mac, 6U);
  pairing_pins[available].pin = pin;
  return BLE_STATUS_SUCCESS;
}

static tBleStatus select_pairing_pin(uint16_t connection_handle, uint32_t *pin) {
  const uint8_t *mac = NULL;
  uint8_t configured = 0U;

  for (uint8_t index = 0U; index < peripheral_connection_count; index++) {
    if (peripheral_connections[index].handle == connection_handle) {
      mac = peripheral_connections[index].mac;
      break;
    }
  }
  if (mac == NULL) {
    return BLE_STATUS_UNKNOWN_CONNECTION_ID;
  }

  for (uint8_t index = 0U; index < PAIRING_PIN_CAPACITY; index++) {
    if ((memcmp(pairing_pins[index].mac, empty_pairing_mac, 6U) != 0) &&
        (memcmp(pairing_pins[index].mac, mac, 6U) == 0)) {
      *pin = pairing_pins[index].pin;
      configured = 1U;
      break;
    }
  }

  if (configured == 0U) {
    uint8_t random_bytes[8];
    uint32_t random_value;
    do {
      tBleStatus status = hci_le_rand(random_bytes);
      if (status != BLE_STATUS_SUCCESS) {
        return status;
      }
      memcpy(&random_value, random_bytes, sizeof(random_value));
      /* Reject the partial top interval before reducing to six decimal digits. */
    } while (random_value >= 4294000000UL);
    *pin = random_value % 1000000U;
    /* TODO: Forward this PIN, peer MAC and connection handle for display. */
  }

  APP_DBG_MSG("Pairing PIN %06lu (%s), peer %02X:%02X:%02X:%02X:%02X:%02X, handle 0x%04X\n",
              (unsigned long)*pin, configured ? "configured" : "random",
              mac[5], mac[4], mac[3], mac[2], mac[1], mac[0], connection_handle);
  return BLE_STATUS_SUCCESS;
}

void APP_BLE_SetPairingMode(uint8_t enabled) {
  APP_BLE_AdvertisingStatus_t previous_advertising_status = advertising_status;

  enabled = (enabled != 0U) ? 1U : 0U;
  if (pairing_mode_enabled != enabled) {
    for (uint8_t index = 0U; index < PAIRING_PIN_CAPACITY; index++) {
      memcpy(pairing_pins[index].mac, empty_pairing_mac, 6U);
      pairing_pins[index].pin = 0U;
    }
  }
  pairing_mode_enabled = (enabled != 0U) ? 1U : 0U;

  if ((advertising_status == APP_BLE_ADV_FAST) || (advertising_status == APP_BLE_ADV_LP)) {
    APP_BLE_Procedure_Gap_Peripheral(PROC_GAP_PERIPH_ADVERTISE_STOP);
    if (advertising_status != APP_BLE_ADV_STOPPED) {
      return;
    }
    APP_BLE_Procedure_Gap_Peripheral((previous_advertising_status == APP_BLE_ADV_FAST)
                                       ? PROC_GAP_PERIPH_ADVERTISE_START_FAST
                                       : PROC_GAP_PERIPH_ADVERTISE_START_LP);
  }
}

tBleStatus APP_BLE_ClearPairingInformation(void) {
  APP_BLE_AdvertisingStatus_t previous_advertising_status = advertising_status;

  /* Connected peers retain their authenticated link after a database erase. */
  if (peripheral_connection_count != 0U) {
    return BLE_STATUS_BUSY;
  }

  if ((advertising_status == APP_BLE_ADV_FAST) || (advertising_status == APP_BLE_ADV_LP)) {
    APP_BLE_Procedure_Gap_Peripheral(PROC_GAP_PERIPH_ADVERTISE_STOP);
    if (advertising_status != APP_BLE_ADV_STOPPED) {
      return BLE_STATUS_BUSY;
    }
  }

  tBleStatus status = aci_gap_clear_security_db();
  if (status == BLE_STATUS_SUCCESS) {
    /* The security database command does not clear these controller lists. */
    status = refresh_bonded_device_lists();
  }

  if ((previous_advertising_status == APP_BLE_ADV_FAST) ||
      (previous_advertising_status == APP_BLE_ADV_LP)) {
    APP_BLE_Procedure_Gap_Peripheral((previous_advertising_status == APP_BLE_ADV_FAST)
                                       ? PROC_GAP_PERIPH_ADVERTISE_START_FAST
                                       : PROC_GAP_PERIPH_ADVERTISE_START_LP);
  }

  return status;
}

/* Call only while advertising is stopped. The resolving list maps a peer's
 * private address to the bonded identity used by the filter accept list. */
static tBleStatus refresh_bonded_device_lists(void) {
  tBleStatus status = hci_le_set_address_resolution_enable(DISABLE);
  if (status != BLE_STATUS_SUCCESS) {
    APP_DBG_MSG("Disable address resolution failed: 0x%02X\n", status);
    return status;
  }

  status = aci_gap_configure_filter_accept_and_resolving_list(0x03U);
  if (status != BLE_STATUS_SUCCESS) {
    APP_DBG_MSG("Load bonded accept/resolving lists failed: 0x%02X\n", status);
    return status;
  }

  status = hci_le_set_address_resolution_enable(ENABLE);
  if (status != BLE_STATUS_SUCCESS) {
    APP_DBG_MSG("Enable address resolution failed: 0x%02X\n", status);
    return status;
  }

  APP_DBG_MSG("Bonded accept/resolving lists loaded; peer address resolution enabled\n");
  Bonded_Device_Entry_t first_bond;
  uint8_t bond_count = 0U;
  status = aci_gap_get_bonded_devices(0U, 1U, &bond_count, &first_bond);
  if (status != BLE_STATUS_SUCCESS) {
    APP_DBG_MSG("Read stored bonds failed: 0x%02X\n", status);
  } else if (bond_count == 0U) {
    APP_DBG_MSG("No stored bonds: locked advertising cannot accept any peers\n");
  }
  return BLE_STATUS_SUCCESS;
}

static tBleStatus validate_connection_request(uint16_t handle) {
  const APP_BLE_Connection_t *connection = find_connection(handle);
  if (connection == NULL) {
    return BLE_STATUS_UNKNOWN_CONNECTION_ID;
  }
  return (connection->status == APP_BLE_DISCONNECTING) ? BLE_STATUS_BUSY : BLE_STATUS_SUCCESS;
}

tBleStatus APP_BLE_Disconnect(uint16_t handle) {
  tBleStatus status = validate_connection_request(handle);
  if (status != BLE_STATUS_SUCCESS) {
    return status;
  }
  status = aci_gap_terminate(handle, BLE_ERROR_TERMINATED_REMOTE_USER);
  if (status == BLE_STATUS_SUCCESS) {
    find_connection(handle)->status = APP_BLE_DISCONNECTING;
  }
  return status;
}

tBleStatus APP_BLE_TogglePhy(uint16_t handle) {
  tBleStatus status = validate_connection_request(handle);
  if (status != BLE_STATUS_SUCCESS) {
    return status;
  }
#if (CFG_BLE_CONTROLLER_2M_CODED_PHY_ENABLED == 1)
  uint8_t phy_tx, phy_rx;
  status = hci_le_read_phy(handle, &phy_tx, &phy_rx);
  if (status != BLE_STATUS_SUCCESS) {
    return status;
  }
  if ((phy_tx == HCI_TX_PHY_LE_2M) && (phy_rx == HCI_RX_PHY_LE_2M)) {
    return hci_le_set_phy(handle, 0, HCI_TX_PHYS_LE_1M_PREF, HCI_RX_PHYS_LE_1M_PREF, 0);
  }
  return hci_le_set_phy(handle, 0, HCI_TX_PHYS_LE_2M_PREF, HCI_RX_PHYS_LE_2M_PREF, 0);
#else
  return BLE_ERROR_UNSUPPORTED_FEATURE;
#endif
}

tBleStatus APP_BLE_ExchangeMtu(uint16_t handle) {
  tBleStatus status = validate_connection_request(handle);
  return (status == BLE_STATUS_SUCCESS) ? aci_gatt_clt_exchange_config(handle) : status;
}

tBleStatus APP_BLE_RequestConnectionParameterUpdate(uint16_t handle) {
  tBleStatus status = validate_connection_request(handle);
  return (status == BLE_STATUS_SUCCESS)
           ? aci_l2cap_connection_parameter_update_req(handle, CONN_INT_MS(1000),
                                                       CONN_INT_MS(1000), 0U, 0x01F4U)
           : status;
}

void APP_BLE_Procedure_Gap_Peripheral(ProcGapPeripheralId_t ProcGapPeripheralId) {
  tBleStatus status;
  uint32_t paramA = ADV_INTERVAL_MIN;
  uint32_t paramB = ADV_INTERVAL_MAX;
  APP_BLE_AdvertisingStatus_t requested_status = APP_BLE_ADV_STOPPED;

  /* First set parameters before calling ACI APIs, only if needed */
  switch (ProcGapPeripheralId) {
    case PROC_GAP_PERIPH_ADVERTISE_START_FAST: {
      paramA = ADV_INTERVAL_MIN;
      paramB = ADV_INTERVAL_MAX;
      requested_status = APP_BLE_ADV_FAST;

      break;
    } /* PROC_GAP_PERIPH_ADVERTISE_START_FAST */
    case PROC_GAP_PERIPH_ADVERTISE_START_LP: {
      paramA = ADV_LP_INTERVAL_MIN;
      paramB = ADV_LP_INTERVAL_MAX;
      requested_status = APP_BLE_ADV_LP;

      break;
    } /* PROC_GAP_PERIPH_ADVERTISE_START_LP */
    case PROC_GAP_PERIPH_ADVERTISE_STOP: {
      requested_status = APP_BLE_ADV_STOPPED;

      break;
    } /* PROC_GAP_PERIPH_ADVERTISE_STOP */

    default:
      break;
  }

  /* Call ACI APIs */
  switch (ProcGapPeripheralId) {
    case PROC_GAP_PERIPH_ADVERTISE_START_FAST:
    case PROC_GAP_PERIPH_ADVERTISE_START_LP: {

      if (peripheral_connection_count >= MAX_PERIPHERAL_CONNECTIONS) {
        APP_DBG_MSG("Advertising start skipped: connection limit reached\n");
        return;
      }
      if (advertising_status != APP_BLE_ADV_STOPPED) {
        APP_BLE_Procedure_Gap_Peripheral(PROC_GAP_PERIPH_ADVERTISE_STOP);
        if (advertising_status != APP_BLE_ADV_STOPPED) {
          return;
        }
      }
      Advertising_Set_Parameters_t Advertising_Set_Parameters = {0};

      /* Include bonds created since boot before switching to a filtered policy.
       * Do not enable advertising if list loading or address resolution fails. */
      status = refresh_bonded_device_lists();
      if (status != BLE_STATUS_SUCCESS) {
        return;
      }

      /* Start Fast or Low Power Advertising */

      /* Set advertising configuration for legacy advertising */
      status = aci_gap_set_advertising_configuration(0,
                                                     pairing_mode_enabled
                                                       ? GAP_MODE_GENERAL_DISCOVERABLE
                                                       : GAP_MODE_NON_DISCOVERABLE,
                                                     ADV_TYPE,
                                                     paramA,
                                                     paramB,
                                                     HCI_ADV_CH_ALL,
                                                     0,
                                                     NULL, /* No peer address */
                                                     pairing_mode_enabled
                                                       ? HCI_ADV_FILTER_NONE
                                                       : HCI_ADV_FILTER_ACCEPT_LIST_SCAN_CONNECT,
                                                     0, /* 0 dBm */
                                                     HCI_PHY_LE_1M, /* Primary advertising PHY */
                                                     0, /* 0 skips */
                                                     HCI_PHY_LE_1M,
                                                     /* Secondary advertising PHY. Not used with legacy advertising. */
                                                     0, /* SID */
                                                     0 /* No scan request notifications */);
      if (status != BLE_STATUS_SUCCESS) {
        APP_DBG_MSG("==>> aci_gap_set_advertising_configuration - fail, result: 0x%02X\n", status);
        return;
      } else {
        APP_DBG_MSG("==>> Success: aci_gap_set_advertising_configuration\n");
      }

      /* Discoverable modes require an unfiltered policy. Keep the Flags AD
       * field consistent with non-discoverable, accept-list-only locked mode. */
      a_AdvData[KTANE_ADV_FLAGS_INDEX] = FLAG_BIT_BR_EDR_NOT_SUPPORTED |
                                         (pairing_mode_enabled ? FLAG_BIT_LE_GENERAL_DISCOVERABLE_MODE : 0U);
      status = aci_gap_set_advertising_data(0, ADV_COMPLETE_DATA, sizeof(a_AdvData), (uint8_t *) a_AdvData);
      if (status != BLE_STATUS_SUCCESS) {
        APP_DBG_MSG("==>> aci_gap_set_advertising_data Failed, result: 0x%02X\n", status);
        return;
      } else {
        APP_DBG_MSG("==>> Success: aci_gap_set_advertising_data\n");
      }

      status = aci_gap_set_scan_response_data(0, sizeof(a_ScanResponseData),
                                              a_ScanResponseData);
      if (status != BLE_STATUS_SUCCESS) {
        APP_DBG_MSG("==>> aci_gap_set_scan_response_data Failed, result: 0x%02X\n", status);
        return;
      } else {
        APP_DBG_MSG("==>> Success: aci_gap_set_scan_response_data\n");
      }

      /* Enable advertising */
      status = aci_gap_set_advertising_enable(ENABLE, 1, &Advertising_Set_Parameters);
      if (status != BLE_STATUS_SUCCESS) {
        APP_DBG_MSG("==>> aci_gap_set_advertising_enable Failed, result: 0x%02X\n", status);
      } else {
        APP_DBG_MSG("==>> Success: aci_gap_set_advertising_enable\n");
        advertising_status = requested_status;
      }
      break;
    }
    case PROC_GAP_PERIPH_ADVERTISE_STOP: {
      status = aci_gap_set_advertising_enable(DISABLE, 0, NULL);
      if (status != BLE_STATUS_SUCCESS) {
        APP_DBG_MSG("Disable advertising - fail, result: 0x%02X\n", status);
      } else {
        APP_DBG_MSG("==>> Disable advertising - Success\n");
        advertising_status = APP_BLE_ADV_STOPPED;
      }
      break;
    } /* PROC_GAP_PERIPH_ADVERTISE_STOP */
    case PROC_GAP_PERIPH_SET_BROADCAST_MODE: {
      break;
    } /* PROC_GAP_PERIPH_SET_BROADCAST_MODE */

    default:
      break;
  }
}

/** \endcond
 */
